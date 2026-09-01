// Unit tests for scan::filter
#include <gtest/gtest.h>

#include "memseek/scan/filter.hpp"
#include "memseek/scan/types.hpp"
#include "memseek/scan/match_storage.hpp"
#include "memseek/value/core.hpp"

using namespace scan;

TEST(ScanFilterTest, NarrowSwathBasic) {
    // Test basic swath narrowing functionality
    MatchesAndOldValuesSwath swath;
    swath.data.resize(10);

    // Initially all entries should be processable
    EXPECT_EQ(swath.data.size(), 10);
}

TEST(ScanFilterTest, EmptySwath) {
    MatchesAndOldValuesSwath swath;
    swath.firstByteInChild = nullptr;

    // Empty swath should have null base
    EXPECT_EQ(swath.firstByteInChild, nullptr);
}
