// Tests for parallel scan consistency vs sequential

import scan.engine;        // runScan, ScanOptions, ScanStats
import scan.co_engine;     // runScanParallel
import scan.match_storage; // MatchesAndOldValuesArray
import scan.types;         // ScanDataType, ScanMatchType
import value;              // UserValue
import value.flags;        // MatchFlags
import core.maps;          // RegionScanLevel

#include <gtest/gtest.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <thread>

// RAII helper to spawn and cleanup an external target process
class ExternalProcess {
   public:
    ExternalProcess() : pid_(0) {
        pid_ = fork();
        if (pid_ == 0) {
            // Child: exec sleep
            execlp("sleep", "sleep", "60", nullptr);
            _exit(127);  // exec failed
        }
        // Parent: give child a moment to start
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    ~ExternalProcess() {
        if (pid_ > 0) {
            kill(pid_, SIGTERM);
            waitpid(pid_, nullptr, 0);
        }
    }

    pid_t pid() const { return pid_; }
    bool valid() const { return pid_ > 0; }

   private:
    pid_t pid_;
};

static auto countMatches(const scan::MatchesAndOldValuesArray& arr)
    -> std::size_t {
    std::size_t total = 0;
    for (const auto& swath : arr.swaths) {
        for (const auto& cell : swath.data) {
            if (cell.matchInfo != MatchFlags::EMPTY) {
                ++total;
            }
        }
    }
    return total;
}

TEST(ScanParallel, ConsistencyWithSequentialAnyNumber) {
    ExternalProcess target;
    ASSERT_TRUE(target.valid()) << "Failed to spawn target process";
    pid_t pid = target.pid();

    std::cout << "[DEBUG] Target PID: " << pid << std::endl;
    std::cout << "[DEBUG] Hardware concurrency: "
              << std::thread::hardware_concurrency() << std::endl;

    ScanOptions opts;
    opts.dataType = ScanDataType::ANY_NUMBER;
    opts.matchType = ScanMatchType::MATCH_ANY;
    opts.step = 16;              // reduce runtime while still exercising logic
    opts.blockSize = 32 * 1024;  // moderate block size
    opts.regionLevel = core::RegionScanLevel::ALL_RW;

    // 扫描外部稳定进程：两次扫描看到相同的内存布局
    // Sequential run
    scan::MatchesAndOldValuesArray seqOut;
    auto seqStatsExp = runScan(pid, opts, nullptr, seqOut);
    ASSERT_TRUE(seqStatsExp.has_value()) << seqStatsExp.error();

    // Parallel run
    scan::MatchesAndOldValuesArray parOut;
    auto parStatsExp = runScanParallel(pid, opts, nullptr, parOut, nullptr);
    ASSERT_TRUE(parStatsExp.has_value()) << parStatsExp.error();

    std::cout << "[DEBUG] Sequential scan:" << std::endl;
    std::cout << "  regionsVisited: " << seqStatsExp->regionsVisited
              << std::endl;
    std::cout << "  bytesScanned: " << seqStatsExp->bytesScanned << std::endl;
    std::cout << "  stats.matches: " << seqStatsExp->matches << std::endl;
    std::cout << "  swaths.size(): " << seqOut.swaths.size() << std::endl;

    std::cout << "[DEBUG] Parallel scan:" << std::endl;
    std::cout << "  regionsVisited: " << parStatsExp->regionsVisited
              << std::endl;
    std::cout << "  bytesScanned: " << parStatsExp->bytesScanned << std::endl;
    std::cout << "  stats.matches: " << parStatsExp->matches << std::endl;
    std::cout << "  swaths.size(): " << parOut.swaths.size() << std::endl;

    const auto seqMatches = countMatches(seqOut);
    const auto parMatches = countMatches(parOut);
    std::cout << "[DEBUG] countMatches(seqOut): " << seqMatches << std::endl;
    std::cout << "[DEBUG] countMatches(parOut): " << parMatches << std::endl;

    // 外部稳定进程：结果应该完全一致
    EXPECT_EQ(seqStatsExp->regionsVisited, parStatsExp->regionsVisited);
    EXPECT_EQ(seqStatsExp->bytesScanned, parStatsExp->bytesScanned);
    EXPECT_EQ(seqMatches, parMatches);

    // swaths 数量应该等于 regionsVisited（每个 region 一个 swath）
    EXPECT_EQ(parOut.swaths.size(), parStatsExp->regionsVisited);
    EXPECT_EQ(seqOut.swaths.size(), seqStatsExp->regionsVisited);
}

TEST(ScanParallel, ConsistencyWithValueEquals) {
    ExternalProcess target;
    ASSERT_TRUE(target.valid()) << "Failed to spawn target process";
    pid_t pid = target.pid();

    // Search for a specific numeric value; use 0 which is common in memory
    UserValue val = UserValue::fromScalar<std::uint64_t>(0);
    val.flags =
        MatchFlags::B8 | MatchFlags::B16 | MatchFlags::B32 | MatchFlags::B64;

    ScanOptions opts;
    opts.dataType = ScanDataType::ANY_NUMBER;
    opts.matchType = ScanMatchType::MATCH_EQUAL_TO;
    opts.step = 32;
    opts.blockSize = 32 * 1024;
    opts.regionLevel = core::RegionScanLevel::ALL_RW;

    // 扫描外部稳定进程
    scan::MatchesAndOldValuesArray seqOut;
    auto seqStatsExp = runScan(pid, opts, &val, seqOut);
    ASSERT_TRUE(seqStatsExp.has_value()) << seqStatsExp.error();

    scan::MatchesAndOldValuesArray parOut;
    auto parStatsExp = runScanParallel(pid, opts, &val, parOut, nullptr);
    ASSERT_TRUE(parStatsExp.has_value()) << parStatsExp.error();

    // 结果应该完全一致
    EXPECT_EQ(seqStatsExp->regionsVisited, parStatsExp->regionsVisited);
    EXPECT_EQ(seqStatsExp->bytesScanned, parStatsExp->bytesScanned);
    EXPECT_EQ(seqStatsExp->matches, parStatsExp->matches);
    EXPECT_EQ(seqOut.swaths.size(), parOut.swaths.size());
}

// 校验模式：顺序与并发严格对等
// 使用外部稳定进程确保内存布局一致
TEST(ScanParallel, StrictEqualityDeepCompare) {
    ExternalProcess target;
    ASSERT_TRUE(target.valid()) << "Failed to spawn target process";
    pid_t pid = target.pid();

    // 测试使用一个常见值匹配，并设定固定步长与块大小
    UserValue val = UserValue::fromScalar<std::uint64_t>(0);
    val.flags =
        MatchFlags::B8 | MatchFlags::B16 | MatchFlags::B32 | MatchFlags::B64;

    ScanOptions opts;
    opts.dataType = ScanDataType::ANY_NUMBER;
    opts.matchType = ScanMatchType::MATCH_EQUAL_TO;
    opts.step = 16;
    opts.blockSize = 32 * 1024;
    opts.regionLevel = core::RegionScanLevel::ALL_RW;

    // 扫描外部稳定进程
    scan::MatchesAndOldValuesArray seqOut;
    auto seqStatsExp = runScan(pid, opts, &val, seqOut);
    ASSERT_TRUE(seqStatsExp.has_value()) << seqStatsExp.error();

    scan::MatchesAndOldValuesArray parOut;
    auto parStatsExp = runScanParallel(pid, opts, &val, parOut, nullptr);
    ASSERT_TRUE(parStatsExp.has_value()) << parStatsExp.error();

    std::cout << "[StrictTest] Sequential: regions="
              << seqStatsExp->regionsVisited
              << " bytes=" << seqStatsExp->bytesScanned
              << " matches=" << seqStatsExp->matches << std::endl;
    std::cout << "[StrictTest] Parallel: regions="
              << parStatsExp->regionsVisited
              << " bytes=" << parStatsExp->bytesScanned
              << " matches=" << parStatsExp->matches << std::endl;

    // 严格对等验证
    EXPECT_EQ(seqStatsExp->regionsVisited, parStatsExp->regionsVisited);
    EXPECT_EQ(seqStatsExp->bytesScanned, parStatsExp->bytesScanned);
    EXPECT_EQ(seqStatsExp->matches, parStatsExp->matches);

    // 深度比较：逐个 swath 对比
    ASSERT_EQ(seqOut.swaths.size(), parOut.swaths.size());
    for (std::size_t i = 0; i < seqOut.swaths.size(); ++i) {
        const auto& seqSwath = seqOut.swaths[i];
        const auto& parSwath = parOut.swaths[i];

        EXPECT_EQ(seqSwath.firstByteInChild, parSwath.firstByteInChild)
            << "Swath " << i << " base address mismatch";
        EXPECT_EQ(seqSwath.data.size(), parSwath.data.size())
            << "Swath " << i << " data size mismatch";

        // 比较每个 cell 的 matchInfo
        for (std::size_t j = 0;
             j < std::min(seqSwath.data.size(), parSwath.data.size()); ++j) {
            EXPECT_EQ(seqSwath.data[j].matchInfo, parSwath.data[j].matchInfo)
                << "Swath " << i << " cell " << j << " matchInfo mismatch";
        }
    }
}
