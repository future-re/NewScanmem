// Unit tests for core::MatchCollector (export-time filtering)
#include <gtest/gtest.h>

#include <string>
#include <vector>

import core.match;         // MatchCollector, MatchCollectionOptions
import core.scanner;       // Scanner
import core.region_filter; // RegionFilterConfig, RegionFilterMode, RegionFilter
import core.maps;          // RegionType
import core.region_classifier; // RegionClassifier
import scan.match_storage; // MatchesAndOldValuesSwath, MatchesAndOldValuesArray
import value.flags;        // MatchFlags

using core::MatchCollectionOptions;
using core::MatchCollector;
using core::RegionClassifier;
using core::RegionFilter;
using core::RegionFilterConfig;
using core::RegionFilterMode;
using core::RegionType;
using core::Scanner;
using scan::MatchesAndOldValuesArray;
using scan::MatchesAndOldValuesSwath;

TEST(MatchCollectorTest, ExportTimeFilterStackAllowed) {
    // Prepare scanner with synthetic matches located on stack
    Scanner scanner(::getpid());
    auto& matches = scanner.getMatches();

    std::array<uint8_t, 8> buf = {10, 20, 30, 40, 50, 60, 70, 80};
    MatchesAndOldValuesSwath swath;
    swath.firstByteInChild = static_cast<void*>(buf.data());
    for (size_t i = 0; i < buf.size(); ++i) {
        swath.addElement(static_cast<void*>(buf.data() + i), buf[i],
                         (i % 2 == 0) ? MatchFlags::B8 : MatchFlags::EMPTY);
    }
    matches.swaths.push_back(swath);

    // Create region classifier for current process
    auto classifierExp = RegionClassifier::create(::getpid());
    ASSERT_TRUE(classifierExp.has_value());
    MatchCollector collector(std::move(classifierExp.value()));

    // Configure export-time filter: allow only stack
    RegionFilterConfig cfg;
    cfg.mode = RegionFilterMode::EXPORT_TIME;
    cfg.filter = RegionFilter::fromTypeNames({"stack"});

    MatchCollectionOptions opts;
    opts.limit = 100;
    opts.collectRegion = true;
    opts.regionFilter = cfg;

    auto [entries, total] = collector.collect(scanner, opts);
    // Only even indices were marked as matches; half of 8 => 4
    EXPECT_EQ(total, 4u);
    ASSERT_EQ(entries.size(), 4u);
    // Verify indices are global across matched cells (0..3)
    EXPECT_EQ(entries[0].index, 0u);
    EXPECT_EQ(entries[1].index, 1u);
    EXPECT_EQ(entries[2].index, 2u);
    EXPECT_EQ(entries[3].index, 3u);
    // Region string should start with "stack" for stack addresses
    for (const auto& e : entries) {
        EXPECT_NE(e.region.find("stack"), std::string::npos);
        EXPECT_EQ(e.value.size(), 1u);
    }
}

TEST(MatchCollectorTest, ExportTimeFilterHeapOnlyDropsStack) {
    Scanner scanner(::getpid());
    auto& matches = scanner.getMatches();

    std::array<uint8_t, 4> buf = {1, 2, 3, 4};
    MatchesAndOldValuesSwath swath;
    swath.firstByteInChild = static_cast<void*>(buf.data());
    for (size_t i = 0; i < buf.size(); ++i) {
        swath.addElement(static_cast<void*>(buf.data() + i), buf[i],
                         MatchFlags::B8);
    }
    matches.swaths.push_back(swath);

    auto classifierExp = RegionClassifier::create(::getpid());
    ASSERT_TRUE(classifierExp.has_value());
    MatchCollector collector(std::move(classifierExp.value()));

    RegionFilterConfig cfg;
    cfg.mode = RegionFilterMode::EXPORT_TIME;
    cfg.filter = RegionFilter::fromTypeNames({"heap"});

    MatchCollectionOptions opts;
    opts.limit = 100;
    opts.collectRegion = true;
    opts.regionFilter = cfg;

    auto [entries, total] = collector.collect(scanner, opts);
    // All addresses are on stack; heap-only filter should drop them
    EXPECT_EQ(total, 0u);
    EXPECT_TRUE(entries.empty());
}
