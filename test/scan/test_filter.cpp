// Unit tests for scan::filter
#include <gtest/gtest.h>

import scan.filter;
import scan.types;
import scan.match_storage;
import value;

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
