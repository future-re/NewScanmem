/**
 * @file test_region_filter.cpp
 * @brief Unit tests for region filtering functionality
 */

#include <gtest/gtest.h>

#include <unordered_set>
#include <vector>

import core.region_filter;
import core.maps;

using namespace core;

/**
 * Test RegionFilter construction and basic operations
 */
TEST(RegionFilterTest, Construction) {
    // Default constructor - allows all types
    RegionFilter filter1;
    EXPECT_FALSE(filter1.isActive());
    EXPECT_TRUE(filter1.isTypeAllowed(RegionType::HEAP));
    EXPECT_TRUE(filter1.isTypeAllowed(RegionType::STACK));
    EXPECT_TRUE(filter1.isTypeAllowed(RegionType::EXE));
    EXPECT_TRUE(filter1.isTypeAllowed(RegionType::CODE));

    // Constructor with specific types
    std::unordered_set<RegionType> allowedTypes{RegionType::HEAP,
                                                RegionType::STACK};
    RegionFilter filter2{allowedTypes};
    EXPECT_TRUE(filter2.isActive());
    EXPECT_TRUE(filter2.isTypeAllowed(RegionType::HEAP));
    EXPECT_TRUE(filter2.isTypeAllowed(RegionType::STACK));
    EXPECT_FALSE(filter2.isTypeAllowed(RegionType::EXE));
    EXPECT_FALSE(filter2.isTypeAllowed(RegionType::CODE));
}

/**
 * Test RegionFilter creation from type names
 */
TEST(RegionFilterTest, FromTypeNames) {
    std::vector<std::string> typeNames{"heap", "stack"};
    auto filter = RegionFilter::fromTypeNames(typeNames);

    EXPECT_TRUE(filter.isActive());
    EXPECT_TRUE(filter.isTypeAllowed(RegionType::HEAP));
    EXPECT_TRUE(filter.isTypeAllowed(RegionType::STACK));
    EXPECT_FALSE(filter.isTypeAllowed(RegionType::EXE));
    EXPECT_FALSE(filter.isTypeAllowed(RegionType::CODE));

    // Test with invalid type names
    std::vector<std::string> invalidNames{"heap", "invalid", "stack"};
    auto filter2 = RegionFilter::fromTypeNames(invalidNames);
    EXPECT_TRUE(filter2.isActive());
    EXPECT_TRUE(filter2.isTypeAllowed(RegionType::HEAP));
    EXPECT_TRUE(filter2.isTypeAllowed(RegionType::STACK));
}

/**
 * Test filtering regions
 */
TEST(RegionFilterTest, FilterRegions) {
    // Create test regions
    std::vector<Region> regions;

    Region heapRegion;
    heapRegion.type = RegionType::HEAP;
    heapRegion.start = reinterpret_cast<void*>(0x1000);  // NOLINT
    heapRegion.size = 4096;
    regions.push_back(heapRegion);

    Region stackRegion;
    stackRegion.type = RegionType::STACK;
    stackRegion.start = reinterpret_cast<void*>(0x2000);  // NOLINT
    stackRegion.size = 4096;
    regions.push_back(stackRegion);

    Region exeRegion;
    exeRegion.type = RegionType::EXE;
    exeRegion.start = reinterpret_cast<void*>(0x3000);  // NOLINT
    exeRegion.size = 4096;
    regions.push_back(exeRegion);

    // Filter to only heap and stack
    std::unordered_set<RegionType> allowedTypes{RegionType::HEAP,
                                                RegionType::STACK};
    RegionFilter filter{allowedTypes};

    auto filtered = filter.filterRegions(regions);
    EXPECT_EQ(filtered.size(), 2);
    EXPECT_EQ(filtered[0].type, RegionType::HEAP);
    EXPECT_EQ(filtered[1].type, RegionType::STACK);

    // Test with no filter (allow all)
    RegionFilter filterAll;
    auto filteredAll = filterAll.filterRegions(regions);
    EXPECT_EQ(filteredAll.size(), 3);
}

/**
 * Test region type operations
 */
TEST(RegionFilterTest, TypeOperations) {
    RegionFilter filter;
    EXPECT_FALSE(filter.isActive());

    // Add types
    filter.addType(RegionType::HEAP);
    EXPECT_TRUE(filter.isActive());
    EXPECT_TRUE(filter.isTypeAllowed(RegionType::HEAP));
    EXPECT_FALSE(filter.isTypeAllowed(RegionType::STACK));

    filter.addType(RegionType::STACK);
    EXPECT_TRUE(filter.isTypeAllowed(RegionType::HEAP));
    EXPECT_TRUE(filter.isTypeAllowed(RegionType::STACK));

    // Remove type
    filter.removeType(RegionType::HEAP);
    EXPECT_FALSE(filter.isTypeAllowed(RegionType::HEAP));
    EXPECT_TRUE(filter.isTypeAllowed(RegionType::STACK));

    // Clear filter
    filter.clear();
    EXPECT_FALSE(filter.isActive());
    EXPECT_TRUE(filter.isTypeAllowed(RegionType::HEAP));
    EXPECT_TRUE(filter.isTypeAllowed(RegionType::STACK));
}

/**
 * Test RegionFilterConfig
 */
TEST(RegionFilterConfigTest, Configuration) {
    RegionFilterConfig config;
    EXPECT_FALSE(config.isEnabled());
    EXPECT_FALSE(config.isScanTimeFilter());
    EXPECT_FALSE(config.isExportTimeFilter());

    // Configure for scan-time filtering
    config.mode = RegionFilterMode::SCAN_TIME;
    config.filter.addType(RegionType::HEAP);
    EXPECT_TRUE(config.isEnabled());
    EXPECT_TRUE(config.isScanTimeFilter());
    EXPECT_FALSE(config.isExportTimeFilter());

    // Configure for export-time filtering
    config.mode = RegionFilterMode::EXPORT_TIME;
    EXPECT_TRUE(config.isEnabled());
    EXPECT_FALSE(config.isScanTimeFilter());
    EXPECT_TRUE(config.isExportTimeFilter());

    // Disable filtering
    config.mode = RegionFilterMode::DISABLED;
    EXPECT_FALSE(config.isEnabled());
}

/**
 * Test toString method
 */
TEST(RegionFilterTest, ToString) {
    RegionFilter filter;
    EXPECT_EQ(filter.toString(), "all regions");

    filter.addType(RegionType::HEAP);
    filter.addType(RegionType::STACK);
    auto str = filter.toString();
    EXPECT_TRUE(str.find("heap") != std::string::npos);
    EXPECT_TRUE(str.find("stack") != std::string::npos);
}

/**
 * Test isRegionAllowed
 */
TEST(RegionFilterTest, IsRegionAllowed) {
    std::unordered_set<RegionType> allowedTypes{RegionType::HEAP};
    RegionFilter filter{allowedTypes};

    Region heapRegion;
    heapRegion.type = RegionType::HEAP;
    EXPECT_TRUE(filter.isRegionAllowed(heapRegion));

    Region stackRegion;
    stackRegion.type = RegionType::STACK;
    EXPECT_FALSE(filter.isRegionAllowed(stackRegion));
}
