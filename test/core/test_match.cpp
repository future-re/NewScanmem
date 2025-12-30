/**
 * @file test_match.cpp
 * @brief Unit tests for core::MatchCollector and MatchEntry
 */

#include <gtest/gtest.h>
#include <unistd.h>

import core.match;
import core.scanner;
import core.region_classifier;
import value;
import scan.types;

using namespace core;

class MatchCollectorTest : public ::testing::Test {
   protected:
    std::unique_ptr<Scanner> scanner;

    void SetUp() override {
        // Create a scanner with some test data (use current process id)
        scanner = std::make_unique<Scanner>(::getpid());
    }
};

TEST_F(MatchCollectorTest, CollectWithoutClassifier) {
    MatchCollector collector;
    MatchCollectionOptions opts;
    opts.limit = 10;
    opts.collectRegion = false;

    auto [entries, total] = collector.collect(*scanner, opts);

    // Empty scanner should return empty results
    EXPECT_EQ(entries.size(), 0);
    EXPECT_EQ(total, 0);
}

TEST_F(MatchCollectorTest, CollectWithLimit) {
    MatchCollector collector;
    MatchCollectionOptions opts;
    opts.limit = 5;

    auto [entries, total] = collector.collect(*scanner, opts);

    // Should respect limit
    EXPECT_LE(entries.size(), opts.limit);
}

TEST_F(MatchCollectorTest, MatchEntryStructure) {
    MatchEntry entry;
    entry.index = 42;
    entry.address = 0x12345678;
    entry.value = {0x01, 0x02, 0x03, 0x04};
    entry.region = "heap";

    EXPECT_EQ(entry.index, 42);
    EXPECT_EQ(entry.address, 0x12345678);
    EXPECT_EQ(entry.value.size(), 4);
    EXPECT_EQ(entry.region, "heap");
}

TEST_F(MatchCollectorTest, CollectionOptionsDefaults) {
    MatchCollectionOptions opts;

    EXPECT_EQ(opts.limit, 100);
    EXPECT_TRUE(opts.collectRegion);
}
