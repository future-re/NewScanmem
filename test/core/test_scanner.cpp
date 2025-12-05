import core.scanner;

using core::Scanner;  // Scanner
import scan.engine;   // ScanOptions
import scan.types;    // ScanDataType / ScanMatchType
import value;         // UserValue

#include <gtest/gtest.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>

// Test strategy:
// - Create a child process with a dedicated mmap page filled with a repeating
//   byte pattern that includes a target value (e.g. 0x2A == 42) multiple times.
// - Parent performs a full scan (MATCH_ANY, INTEGER_8) and records match count.
// - Parent then performs a filtered scan (MATCHEQUALTO, INTEGER_8, value=42)
//   and expects the match count to reduce (or at least not increase).
// - Parent performs another full scan (MATCH_ANY) and expects match count to
//   reset (be >= filtered count).
// - Additional test ensures calling filtered scan first returns an error.
//
// NOTE: The scan engine scans ALL_RW regions of the child, not just our page,
// so we do not assert exact counts, only relational properties (narrowing &
// reset behavior).

static int g_pipe_write_fd_scanner = -1;  // write end used by child

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
        // Parent
        ::close(pipefd[1]);  // close write end
        // Read one pointer (base address) from child
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
        if (block == MAP_FAILED) {
            _exit(1);
        }
        auto* bytes = static_cast<uint8_t*>(block);
        // Fill first 256 bytes with pattern containing many 42 values
        const uint8_t pattern[] = {42, 7, 42, 9, 11, 42, 13, 15};
        for (size_t i = 0; i < 256; ++i) {
            bytes[i] = pattern[i % (sizeof(pattern))];
        }
        // Send base address to parent
        auto out = reinterpret_cast<uintptr_t>(block);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
        ::write(g_pipe_write_fd_scanner, &out, sizeof(out));
        // Keep child alive; lightly touch memory
        while (true) {
            bytes[0] = bytes[0];
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    pid_t m_childPid{-1};
    void* m_regionBase{nullptr};
};

// Test: filtered scan error when no prior full scan
TEST(ScannerStandaloneTest, FilteredScanWithoutInitialFull) {
    Scanner scanner(::getpid());  // current process
    ScanOptions opts;             // defaults MATCH_ANY ANYNUMBER
    auto filtered = scanner.performFilteredScan(opts);
    EXPECT_FALSE(filtered.has_value());
}

// Test: full scan then filtered scan narrowing and reset on new full scan
TEST_F(ScannerTest, FullThenFilteredAndReset) {
    ASSERT_GT(childPid(), 0);
    Scanner scanner(childPid());

    // Full scan (match any int8)
    ScanOptions fullOpts;
    fullOpts.dataType = ScanDataType::INTEGER_8;
    fullOpts.matchType = ScanMatchType::MATCH_ANY;
    auto fullStatsExp = scanner.performScan(fullOpts);
    ASSERT_TRUE(fullStatsExp.has_value()) << "Full scan failed";
    auto fullCount = scanner.getMatchCount();
    ASSERT_GT(fullCount, 0U) << "Full scan should produce matches";

    // Filtered scan (only bytes equal to 42)
    UserValue val = UserValue::fromScalar<int8_t>(42);
    ScanOptions filteredOpts;
    filteredOpts.dataType = ScanDataType::INTEGER_8;
    filteredOpts.matchType = ScanMatchType::MATCH_EQUAL_TO;
    auto filteredStatsExp = scanner.performFilteredScan(filteredOpts, &val);
    ASSERT_TRUE(filteredStatsExp.has_value()) << "Filtered scan failed";
    auto narrowedCount = scanner.getMatchCount();
    EXPECT_GT(narrowedCount, 0U) << "Should retain some matches for value 42";
    EXPECT_LE(narrowedCount, fullCount)
        << "Filtered scan should not increase matches";

    // Another full scan should reset matches to a (likely) larger count
    auto fullAgainExp = scanner.performScan(fullOpts);
    ASSERT_TRUE(fullAgainExp.has_value()) << "Second full scan failed";
    auto fullAgainCount = scanner.getMatchCount();
    EXPECT_GE(fullAgainCount, narrowedCount)
        << "Full scan should reset/widen matches";
}
