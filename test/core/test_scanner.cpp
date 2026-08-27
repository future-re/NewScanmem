#include "newscanmem/core/scanner.hpp"

using core::Scanner;  // Scanner
#include "newscanmem/scan/engine.hpp"
#include "newscanmem/scan/types.hpp"
#include "newscanmem/value/core.hpp"

#include <gtest/gtest.h>

#include <thread>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>

static int g_pipe_write_fd_scanner = -1;

class ScannerTest : public ::testing::Test {
   protected:
    void SetUp() override {
        int pipefd[2];
        ASSERT_EQ(::pipe(pipefd), 0) << "pipe creation failed";
        g_pipe_write_fd_scanner = pipefd[1];
        m_childPid = ::fork();
        ASSERT_NE(m_childPid, -1) << "fork failed";
        if (m_childPid == 0) {
            ::close(pipefd[0]);
            runChild();
            _exit(0);
        }
        ::close(pipefd[1]);
        uintptr_t addrValue = 0;
        std::size_t need = sizeof(uintptr_t);
        std::size_t got = 0;
        while (got < need) {
            ssize_t r =
                ::read(pipefd[0], reinterpret_cast<char*>(&addrValue) + got,
                       need - got);
            ASSERT_GE(r, 0) << "read failed: " << strerror(errno);
            if (r == 0) break;
            got += static_cast<std::size_t>(r);
        }
        ASSERT_EQ(got, need) << "Did not receive address from child";
        ::close(pipefd[0]);
        m_regionBase = reinterpret_cast<void*>(addrValue);
    }

    void TearDown() override {
        if (m_childPid > 0) {
            ::kill(m_childPid, SIGTERM);
            ::waitpid(m_childPid, nullptr, 0);
        }
    }

    [[nodiscard]] auto childPid() const -> pid_t { return m_childPid; }

   private:
    static void runChild() {
        const auto pageSize = static_cast<size_t>(::sysconf(_SC_PAGESIZE));
        void* block = ::mmap(nullptr, pageSize, PROT_READ | PROT_WRITE,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (block == MAP_FAILED) _exit(1);
        auto* bytes = static_cast<uint8_t*>(block);
        const uint8_t pattern[] = {42, 7, 42, 9, 11, 42, 13, 15};
        for (size_t i = 0; i < 256; ++i) bytes[i] = pattern[i % sizeof(pattern)];
        auto out = reinterpret_cast<uintptr_t>(block);
        ::write(g_pipe_write_fd_scanner, &out, sizeof(out));
        while (true) {
            bytes[0] = bytes[0];
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    pid_t m_childPid{-1};
    void* m_regionBase{nullptr};
};

TEST(ScannerStandaloneTest, FilteredScanWithoutInitialFull) {
    Scanner scanner(::getpid());
    ScanOptions opts;
    auto filtered = scanner.filter(opts);
    EXPECT_FALSE(filtered.success);
}

TEST(ScannerStandaloneTest, MatchCountCacheInvalidatesOnMutableAccess) {
    Scanner scanner(::getpid());
    scan::MatchesAndOldValuesSwath swath;
    swath.firstByteInChild = reinterpret_cast<void*>(0x1000);
    swath.data.push_back(
        {.oldByte = 1, .matchInfo = MatchFlags::B8, .matchLength = 1});
    scanner.getMatches().addSwath(swath);

    EXPECT_EQ(scanner.getMatchCount(), 1U);
    EXPECT_TRUE(scanner.hasMatches());

    scanner.getMatches().swaths.front().data.front().matchInfo = MatchFlags::EMPTY;

    EXPECT_EQ(scanner.getMatchCount(), 0U);
    EXPECT_FALSE(scanner.hasMatches());
}

TEST_F(ScannerTest, FullThenFilteredAndReset) {
    ASSERT_GT(childPid(), 0);
    Scanner scanner(childPid());

    ScanOptions fullOpts;
    fullOpts.dataType = ScanDataType::INTEGER_8;
    fullOpts.matchType = ScanMatchType::MATCH_ANY;
    auto fullResult = scanner.snapshot(fullOpts);
    ASSERT_TRUE(fullResult.success) << "Full scan failed";
    auto fullCount = scanner.getMatchCount();
    ASSERT_GT(fullCount, 0U) << "Full scan should produce matches";

    UserValue val = UserValue::of<int8_t>(42);
    ScanOptions filteredOpts;
    filteredOpts.dataType = ScanDataType::INTEGER_8;
    filteredOpts.matchType = ScanMatchType::MATCH_EQUAL_TO;
    auto filteredResult = scanner.filter(filteredOpts, val, true);
    ASSERT_TRUE(filteredResult.success) << "Filtered scan failed";
    auto narrowedCount = scanner.getMatchCount();
    EXPECT_GT(narrowedCount, 0U) << "Should retain some matches for value 42";
    EXPECT_LE(narrowedCount, fullCount)
        << "Filtered scan should not increase matches";

    auto fullAgain = scanner.snapshot(fullOpts);
    ASSERT_TRUE(fullAgain.success) << "Second full scan failed";
    auto fullAgainCount = scanner.getMatchCount();
    EXPECT_GE(fullAgainCount, narrowedCount)
        << "Full scan should reset/widen matches";
}
