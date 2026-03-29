// Unit tests for scan::factory
#include <gtest/gtest.h>

import scan.factory;
import scan.types;
import value.core;
import value.flags;

TEST(ScanFactoryTest, GetRoutineInteger8) {
    auto routine =
        scan::makeScanRoutine(ScanDataType::INTEGER_8,
                              ScanMatchType::MATCH_ANY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineInteger16) {
    auto routine =
        scan::makeScanRoutine(ScanDataType::INTEGER_16,
                              ScanMatchType::MATCH_ANY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineInteger32) {
    auto routine =
        scan::makeScanRoutine(ScanDataType::INTEGER_32,
                              ScanMatchType::MATCH_ANY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineInteger64) {
    auto routine =
        scan::makeScanRoutine(ScanDataType::INTEGER_64,
                              ScanMatchType::MATCH_ANY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineFloat32) {
    auto routine =
        scan::makeScanRoutine(ScanDataType::FLOAT_32,
                              ScanMatchType::MATCH_ANY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineFloat64) {
    auto routine =
        scan::makeScanRoutine(ScanDataType::FLOAT_64,
                              ScanMatchType::MATCH_ANY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineByteArray) {
    auto routine =
        scan::makeScanRoutine(ScanDataType::BYTE_ARRAY,
                              ScanMatchType::MATCH_ANY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineString) {
    auto routine = scan::makeScanRoutine(ScanDataType::STRING,
                                         ScanMatchType::MATCH_ANY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineAnyInteger) {
    auto routine =
        scan::makeScanRoutine(ScanDataType::ANY_INTEGER,
                              ScanMatchType::MATCH_ANY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineAnyFloat) {
    auto routine =
        scan::makeScanRoutine(ScanDataType::ANY_FLOAT,
                              ScanMatchType::MATCH_ANY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, GetRoutineAnyNumber) {
    auto routine =
        scan::makeScanRoutine(ScanDataType::ANY_NUMBER,
                              ScanMatchType::MATCH_ANY, false);
    EXPECT_TRUE(static_cast<bool>(routine));
}

TEST(ScanFactoryTest, ChooseScanRoutineValid) {
    bool result = scan::isRoutineAvailable(ScanDataType::INTEGER_32,
                                           ScanMatchType::MATCH_ANY);
    EXPECT_TRUE(result);
}

TEST(ScanFactoryTest, ReverseEndianness) {
    auto routine1 =
        scan::makeScanRoutine(ScanDataType::INTEGER_32,
                              ScanMatchType::MATCH_ANY, false);
    auto routine2 =
        scan::makeScanRoutine(ScanDataType::INTEGER_32,
                              ScanMatchType::MATCH_ANY, true);

    // Both should be valid
    EXPECT_TRUE(static_cast<bool>(routine1));
    EXPECT_TRUE(static_cast<bool>(routine2));
}
