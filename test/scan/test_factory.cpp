// Unit tests for scan::factory
#include <gtest/gtest.h>

import scan.factory;
import scan.types;
import value;

TEST(ScanFactoryTest, GetRoutineInteger8) {
    auto routine =
        smGetScanroutine(ScanDataType::INTEGER_8, ScanMatchType::MATCH_ANY,
                         MatchFlags::EMPTY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineInteger16) {
    auto routine =
        smGetScanroutine(ScanDataType::INTEGER_16, ScanMatchType::MATCH_ANY,
                         MatchFlags::EMPTY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineInteger32) {
    auto routine =
        smGetScanroutine(ScanDataType::INTEGER_32, ScanMatchType::MATCH_ANY,
                         MatchFlags::EMPTY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineInteger64) {
    auto routine =
        smGetScanroutine(ScanDataType::INTEGER_64, ScanMatchType::MATCH_ANY,
                         MatchFlags::EMPTY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineFloat32) {
    auto routine =
        smGetScanroutine(ScanDataType::FLOAT_32, ScanMatchType::MATCH_ANY,
                         MatchFlags::EMPTY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineFloat64) {
    auto routine =
        smGetScanroutine(ScanDataType::FLOAT_64, ScanMatchType::MATCH_ANY,
                         MatchFlags::EMPTY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineByteArray) {
    auto routine =
        smGetScanroutine(ScanDataType::BYTE_ARRAY, ScanMatchType::MATCH_ANY,
                         MatchFlags::EMPTY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineString) {
    auto routine =
        smGetScanroutine(ScanDataType::STRING, ScanMatchType::MATCH_ANY,
                         MatchFlags::EMPTY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineAnyInteger) {
    auto routine =
        smGetScanroutine(ScanDataType::ANY_INTEGER, ScanMatchType::MATCH_ANY,
                         MatchFlags::EMPTY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineAnyFloat) {
    auto routine =
        smGetScanroutine(ScanDataType::ANY_FLOAT, ScanMatchType::MATCH_ANY,
                         MatchFlags::EMPTY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineAnyNumber) {
    auto routine =
        smGetScanroutine(ScanDataType::ANY_NUMBER, ScanMatchType::MATCH_ANY,
                         MatchFlags::EMPTY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, ChooseScanRoutineValid) {
    UserValue uv;
    uv.flags = MatchFlags::B32;

    bool result = smChooseScanroutine(ScanDataType::INTEGER_32,
                                      ScanMatchType::MATCH_ANY, uv, false);
    EXPECT_TRUE(result);
}

TEST(ScanFactoryTest, ReverseEndianness) {
    auto routine1 =
        smGetScanroutine(ScanDataType::INTEGER_32, ScanMatchType::MATCH_ANY,
                         MatchFlags::EMPTY, false);
    auto routine2 =
        smGetScanroutine(ScanDataType::INTEGER_32, ScanMatchType::MATCH_ANY,
                         MatchFlags::EMPTY, true);

    // Both should be valid
    EXPECT_TRUE(static_cast<bool>(routine1));
    EXPECT_TRUE(static_cast<bool>(routine2));
}
