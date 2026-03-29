// Unit tests for scan.numeric module - focus on nullptr safety

#include <gtest/gtest.h>

#include <cstdint>
#include <span>
#include <vector>

import scan.numeric;
import scan.routine;
import scan.types;
import value.core;
import value.flags;

class ScanNumericTest : public ::testing::Test {
   protected:
    Value m_mem;
};

// Test numericMatchCore with nullptr saveFlags
TEST_F(ScanNumericTest, NumericMatchCoreWithNullSaveFlagsDoesNotCrash) {
    std::vector<uint8_t> data = {42, 0, 0, 0};  // int32 = 42
    m_mem = Value(data);

    UserValue userValue = UserValue::fromScalar<int32_t>(42);

    // Should not crash with nullptr saveFlags
    unsigned int result = numericMatchCore<int32_t>(
        ScanMatchType::MATCH_EQUAL_TO, 42, nullptr, &userValue, nullptr);

    EXPECT_EQ(result, sizeof(int32_t));
}

// Test makeNumericScanRoutine on a canonical ScanContext
TEST_F(ScanNumericTest, MakeNumericScanRoutineMatchesAnyValue) {
    std::vector<uint8_t> data = {100, 0, 0, 0, 0, 0, 0, 0};  // int64 = 100
    m_mem = Value(data);

    auto routine =
        makeNumericScanRoutine<int64_t>(ScanMatchType::MATCH_ANY, false);

    auto ctx = scan::makeScanContext(
        std::span<const uint8_t>(m_mem.bytes.data(), m_mem.bytes.size()),
        nullptr, nullptr, MatchFlags::EMPTY, false);
    auto result = routine(ctx);

    EXPECT_EQ(result.matchLength, sizeof(int64_t));
    EXPECT_EQ(result.matchedFlag, MatchFlags::B64);
}

TEST_F(ScanNumericTest, MakeAnyIntegerScanRoutineMatches) {
    std::vector<uint8_t> data = {0xFF, 0x00};  // uint16 = 255
    m_mem = Value(data);

    auto routine = makeAnyIntegerScanRoutine(ScanMatchType::MATCH_ANY, false);
    auto ctx = scan::makeScanContext(
        std::span<const uint8_t>(m_mem.bytes.data(), m_mem.bytes.size()),
        nullptr, nullptr, MatchFlags::EMPTY, false);
    auto result = routine(ctx);

    EXPECT_GT(result.matchLength, 0U);
}

TEST_F(ScanNumericTest, MakeAnyFloatScanRoutineMatches) {
    std::vector<uint8_t> data = {0, 0, 0x80, 0x3F};  // float = 1.0
    m_mem = Value(data);

    auto routine = makeAnyFloatScanRoutine(ScanMatchType::MATCH_ANY, false);
    auto ctx = scan::makeScanContext(
        std::span<const uint8_t>(m_mem.bytes.data(), m_mem.bytes.size()),
        nullptr, nullptr, MatchFlags::EMPTY, false);
    auto result = routine(ctx);

    EXPECT_EQ(result.matchLength, sizeof(float));
}

TEST_F(ScanNumericTest, MakeAnyNumberScanRoutineMatches) {
    std::vector<uint8_t> data = {42};  // uint8 = 42
    m_mem = Value(data);

    auto routine = makeAnyNumberScanRoutine(ScanMatchType::MATCH_ANY, false);
    auto ctx = scan::makeScanContext(
        std::span<const uint8_t>(m_mem.bytes.data(), m_mem.bytes.size()),
        nullptr, nullptr, MatchFlags::EMPTY, false);
    auto result = routine(ctx);

    EXPECT_GT(result.matchLength, 0U);
}

TEST_F(ScanNumericTest, NumericRoutineSetsFlags) {
    std::vector<uint8_t> data = {42, 0};  // uint16 = 42
    m_mem = Value(data);

    auto routine =
        makeNumericScanRoutine<uint16_t>(ScanMatchType::MATCH_ANY, false);
    auto ctx = scan::makeScanContext(
        std::span<const uint8_t>(m_mem.bytes.data(), m_mem.bytes.size()),
        nullptr, nullptr, MatchFlags::EMPTY, false);
    auto result = routine(ctx);

    EXPECT_EQ(result.matchLength, sizeof(uint16_t));
    EXPECT_EQ(result.matchedFlag, MatchFlags::B16);
}
