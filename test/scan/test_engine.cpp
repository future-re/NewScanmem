// Unit tests for scan::engine
#include <gtest/gtest.h>

#include "memseek/scan/engine.hpp"
#include "memseek/scan/types.hpp"

TEST(ScanEngineTest, ScanOptionsDefaults) {
    ScanOptions opts;

    EXPECT_EQ(opts.dataType, ScanDataType::ANY_NUMBER);
    EXPECT_EQ(opts.matchType, ScanMatchType::MATCH_ANY);
    EXPECT_FALSE(opts.reverseEndianness);
    EXPECT_EQ(opts.step, 1);
    EXPECT_EQ(opts.blockSize, ScanOptions::BLOCK_SIZE);
}

TEST(ScanEngineTest, ScanStatsInitialization) {
    ScanStats stats;

    EXPECT_EQ(stats.regionsVisited, 0);
}

TEST(ScanEngineTest, CustomBlockSize) {
    ScanOptions opts;
    opts.blockSize = 128 * 1024;

    EXPECT_EQ(opts.blockSize, 128 * 1024);
}

TEST(ScanEngineTest, CustomStepSize) {
    ScanOptions opts;
    opts.step = 4;  // Step by 4 bytes (e.g., for aligned int32 scan)

    EXPECT_EQ(opts.step, 4);
}
