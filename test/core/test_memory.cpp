/**
 * @file test_memory.cpp
 * @brief Unit tests for memory scanning logic
 *
 * Tests the memory scanning functionality including:
 * - Numeric value matching (8/16/32/64 bit integers and floats)
 * - Different match types (equal, greater than, less than, range, etc.)
 * - Memory reader/writer operations
 * - Scan type utilities
 */

import scan.types;
import scan.numeric;
import scan.read_helpers;
import utils.mem64;
import value;
import value.flags;

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>

// =============================================================================
// Test fixtures and helper functions
// =============================================================================

namespace {

// Helper to create a UserValue with a specific integer value
template <typename T>
UserValue makeUserValue(T value) {
    UserValue uv;
    if constexpr (std::is_same_v<T, int8_t>) {
        uv.s8 = value;
        uv.flags = MatchFlags::B8;
    } else if constexpr (std::is_same_v<T, uint8_t>) {
        uv.u8 = value;
        uv.flags = MatchFlags::B8;
    } else if constexpr (std::is_same_v<T, int16_t>) {
        uv.s16 = value;
        uv.flags = MatchFlags::B16;
    } else if constexpr (std::is_same_v<T, uint16_t>) {
        uv.u16 = value;
        uv.flags = MatchFlags::B16;
    } else if constexpr (std::is_same_v<T, int32_t>) {
        uv.s32 = value;
        uv.flags = MatchFlags::B32;
    } else if constexpr (std::is_same_v<T, uint32_t>) {
        uv.u32 = value;
        uv.flags = MatchFlags::B32;
    } else if constexpr (std::is_same_v<T, int64_t>) {
        uv.s64 = value;
        uv.flags = MatchFlags::B64;
    } else if constexpr (std::is_same_v<T, uint64_t>) {
        uv.u64 = value;
        uv.flags = MatchFlags::B64;
    } else if constexpr (std::is_same_v<T, float>) {
        uv.f32 = value;
        uv.flags = MatchFlags::B32;
    } else if constexpr (std::is_same_v<T, double>) {
        uv.f64 = value;
        uv.flags = MatchFlags::B64;
    }
    return uv;
}

// Helper to create a UserValue with range values
template <typename T>
UserValue makeRangeUserValue(T low, T high) {
    UserValue uv;
    if constexpr (std::is_same_v<T, int32_t>) {
        uv.s32 = low;
        uv.s32h = high;
        uv.flags = MatchFlags::B32;
    } else if constexpr (std::is_same_v<T, uint32_t>) {
        uv.u32 = low;
        uv.u32h = high;
        uv.flags = MatchFlags::B32;
    } else if constexpr (std::is_same_v<T, float>) {
        uv.f32 = low;
        uv.f32h = high;
        uv.flags = MatchFlags::B32;
    } else if constexpr (std::is_same_v<T, double>) {
        uv.f64 = low;
        uv.f64h = high;
        uv.flags = MatchFlags::B64;
    }
    return uv;
}

}  // namespace

// =============================================================================
// Numeric Matching Core Tests
// =============================================================================

TEST(NumericMatchTest, MatchEqualTo_Int32) {
    std::array<uint8_t, 4> buffer{};
    int32_t testValue = 42;
    std::memcpy(buffer.data(), &testValue, sizeof(testValue));

    Mem64 mem{buffer.data(), buffer.size()};
    MatchFlags saveFlags = MatchFlags::EMPTY;
    UserValue userValue = makeUserValue<int32_t>(42);

    unsigned int result = numericMatchCore<int32_t>(
        ScanMatchType::MATCHEQUALTO, testValue, nullptr, &userValue, &saveFlags);

    EXPECT_EQ(result, sizeof(int32_t));
    EXPECT_EQ(saveFlags, MatchFlags::B32);
}

TEST(NumericMatchTest, MatchEqualTo_Int32_NoMatch) {
    int32_t testValue = 42;
    MatchFlags saveFlags = MatchFlags::EMPTY;
    UserValue userValue = makeUserValue<int32_t>(100);

    unsigned int result = numericMatchCore<int32_t>(
        ScanMatchType::MATCHEQUALTO, testValue, nullptr, &userValue, &saveFlags);

    EXPECT_EQ(result, 0U);
}

TEST(NumericMatchTest, MatchGreaterThan_Int32) {
    int32_t testValue = 100;
    MatchFlags saveFlags = MatchFlags::EMPTY;
    UserValue userValue = makeUserValue<int32_t>(50);

    unsigned int result = numericMatchCore<int32_t>(
        ScanMatchType::MATCHGREATERTHAN, testValue, nullptr, &userValue,
        &saveFlags);

    EXPECT_EQ(result, sizeof(int32_t));
    EXPECT_EQ(saveFlags, MatchFlags::B32);
}

TEST(NumericMatchTest, MatchLessThan_Int32) {
    int32_t testValue = 25;
    MatchFlags saveFlags = MatchFlags::EMPTY;
    UserValue userValue = makeUserValue<int32_t>(50);

    unsigned int result = numericMatchCore<int32_t>(
        ScanMatchType::MATCHLESSTHAN, testValue, nullptr, &userValue, &saveFlags);

    EXPECT_EQ(result, sizeof(int32_t));
    EXPECT_EQ(saveFlags, MatchFlags::B32);
}

TEST(NumericMatchTest, MatchNotEqualTo_Int32) {
    int32_t testValue = 42;
    MatchFlags saveFlags = MatchFlags::EMPTY;
    UserValue userValue = makeUserValue<int32_t>(100);

    unsigned int result = numericMatchCore<int32_t>(
        ScanMatchType::MATCHNOTEQUALTO, testValue, nullptr, &userValue,
        &saveFlags);

    EXPECT_EQ(result, sizeof(int32_t));
    EXPECT_EQ(saveFlags, MatchFlags::B32);
}

TEST(NumericMatchTest, MatchAny_Int32) {
    int32_t testValue = 42;
    MatchFlags saveFlags = MatchFlags::EMPTY;

    unsigned int result = numericMatchCore<int32_t>(
        ScanMatchType::MATCHANY, testValue, nullptr, nullptr, &saveFlags);

    EXPECT_EQ(result, sizeof(int32_t));
    EXPECT_EQ(saveFlags, MatchFlags::B32);
}

TEST(NumericMatchTest, MatchRange_Int32_InRange) {
    int32_t testValue = 75;
    MatchFlags saveFlags = MatchFlags::EMPTY;
    UserValue userValue = makeRangeUserValue<int32_t>(50, 100);

    unsigned int result = numericMatchCore<int32_t>(
        ScanMatchType::MATCHRANGE, testValue, nullptr, &userValue, &saveFlags);

    EXPECT_EQ(result, sizeof(int32_t));
    EXPECT_EQ(saveFlags, MatchFlags::B32);
}

TEST(NumericMatchTest, MatchRange_Int32_OutOfRange) {
    int32_t testValue = 150;
    MatchFlags saveFlags = MatchFlags::EMPTY;
    UserValue userValue = makeRangeUserValue<int32_t>(50, 100);

    unsigned int result = numericMatchCore<int32_t>(
        ScanMatchType::MATCHRANGE, testValue, nullptr, &userValue, &saveFlags);

    EXPECT_EQ(result, 0U);
}

// =============================================================================
// Float Matching Tests
// =============================================================================

TEST(NumericMatchTest, MatchEqualTo_Float) {
    float testValue = 3.14159F;
    MatchFlags saveFlags = MatchFlags::EMPTY;
    UserValue userValue = makeUserValue<float>(3.14159F);

    unsigned int result = numericMatchCore<float>(
        ScanMatchType::MATCHEQUALTO, testValue, nullptr, &userValue, &saveFlags);

    EXPECT_EQ(result, sizeof(float));
    EXPECT_EQ(saveFlags, MatchFlags::B32);
}

TEST(NumericMatchTest, MatchEqualTo_Float_WithTolerance) {
    float testValue = 3.14159F;
    MatchFlags saveFlags = MatchFlags::EMPTY;
    // Slightly different value within tolerance
    UserValue userValue = makeUserValue<float>(3.141590001F);

    unsigned int result = numericMatchCore<float>(
        ScanMatchType::MATCHEQUALTO, testValue, nullptr, &userValue, &saveFlags);

    EXPECT_EQ(result, sizeof(float));
}

TEST(NumericMatchTest, MatchGreaterThan_Double) {
    double testValue = 3.14159;
    MatchFlags saveFlags = MatchFlags::EMPTY;
    UserValue userValue = makeUserValue<double>(2.0);

    unsigned int result = numericMatchCore<double>(
        ScanMatchType::MATCHGREATERTHAN, testValue, nullptr, &userValue,
        &saveFlags);

    EXPECT_EQ(result, sizeof(double));
    EXPECT_EQ(saveFlags, MatchFlags::B64);
}

// =============================================================================
// 8-bit and 16-bit Integer Tests
// =============================================================================

TEST(NumericMatchTest, MatchEqualTo_Int8) {
    int8_t testValue = -42;
    MatchFlags saveFlags = MatchFlags::EMPTY;
    UserValue userValue = makeUserValue<int8_t>(-42);

    unsigned int result = numericMatchCore<int8_t>(
        ScanMatchType::MATCHEQUALTO, testValue, nullptr, &userValue, &saveFlags);

    EXPECT_EQ(result, sizeof(int8_t));
    EXPECT_EQ(saveFlags, MatchFlags::B8);
}

TEST(NumericMatchTest, MatchEqualTo_UInt16) {
    uint16_t testValue = 65535U;
    MatchFlags saveFlags = MatchFlags::EMPTY;
    UserValue userValue = makeUserValue<uint16_t>(65535U);

    unsigned int result = numericMatchCore<uint16_t>(
        ScanMatchType::MATCHEQUALTO, testValue, nullptr, &userValue, &saveFlags);

    EXPECT_EQ(result, sizeof(uint16_t));
    EXPECT_EQ(saveFlags, MatchFlags::B16);
}

// =============================================================================
// Numeric Routine Execution Tests  
// =============================================================================

TEST(NumericRoutineTest, MakeNumericRoutine_Int32_Match) {
    auto routine = makeNumericRoutine<int32_t>(ScanMatchType::MATCHEQUALTO, false);
    ASSERT_TRUE(static_cast<bool>(routine));

    std::array<uint8_t, 8> buffer{};
    int32_t testValue = 12345;
    std::memcpy(buffer.data(), &testValue, sizeof(testValue));

    Mem64 mem{buffer.data(), buffer.size()};
    MatchFlags saveFlags = MatchFlags::EMPTY;
    UserValue userValue = makeUserValue<int32_t>(12345);

    unsigned int result =
        routine(&mem, buffer.size(), nullptr, &userValue, &saveFlags);

    EXPECT_EQ(result, sizeof(int32_t));
    EXPECT_EQ(saveFlags, MatchFlags::B32);
}

TEST(NumericRoutineTest, MakeNumericRoutine_Int32_NoMatch) {
    auto routine = makeNumericRoutine<int32_t>(ScanMatchType::MATCHEQUALTO, false);
    ASSERT_TRUE(static_cast<bool>(routine));

    std::array<uint8_t, 8> buffer{};
    int32_t testValue = 12345;
    std::memcpy(buffer.data(), &testValue, sizeof(testValue));

    Mem64 mem{buffer.data(), buffer.size()};
    MatchFlags saveFlags = MatchFlags::EMPTY;
    UserValue userValue = makeUserValue<int32_t>(99999);

    unsigned int result =
        routine(&mem, buffer.size(), nullptr, &userValue, &saveFlags);

    EXPECT_EQ(result, 0U);
}

TEST(NumericRoutineTest, MakeAnyIntegerRoutine) {
    auto routine = makeAnyIntegerRoutine(ScanMatchType::MATCHANY, false);
    ASSERT_TRUE(static_cast<bool>(routine));

    std::array<uint8_t, 8> buffer{};
    int64_t testValue = 123456789;
    std::memcpy(buffer.data(), &testValue, sizeof(testValue));

    Mem64 mem{buffer.data(), buffer.size()};
    MatchFlags saveFlags = MatchFlags::EMPTY;

    unsigned int result =
        routine(&mem, buffer.size(), nullptr, nullptr, &saveFlags);

    // Should match with some width
    EXPECT_GT(result, 0U);
}

TEST(NumericRoutineTest, MakeAnyFloatRoutine) {
    auto routine = makeAnyFloatRoutine(ScanMatchType::MATCHANY, false);
    ASSERT_TRUE(static_cast<bool>(routine));

    std::array<uint8_t, 8> buffer{};
    double testValue = 3.14159;
    std::memcpy(buffer.data(), &testValue, sizeof(testValue));

    Mem64 mem{buffer.data(), buffer.size()};
    MatchFlags saveFlags = MatchFlags::EMPTY;

    unsigned int result =
        routine(&mem, buffer.size(), nullptr, nullptr, &saveFlags);

    // Should match with some width
    EXPECT_GT(result, 0U);
}

TEST(NumericRoutineTest, MakeAnyNumberRoutine) {
    auto routine = makeAnyNumberRoutine(ScanMatchType::MATCHANY, false);
    ASSERT_TRUE(static_cast<bool>(routine));

    std::array<uint8_t, 8> buffer{};
    double testValue = 3.14159;
    std::memcpy(buffer.data(), &testValue, sizeof(testValue));

    Mem64 mem{buffer.data(), buffer.size()};
    MatchFlags saveFlags = MatchFlags::EMPTY;

    unsigned int result =
        routine(&mem, buffer.size(), nullptr, nullptr, &saveFlags);

    // Should match with some width
    EXPECT_GT(result, 0U);
}

// =============================================================================
// Read Helpers Tests
// =============================================================================

TEST(ReadHelpersTest, FlagForType) {
    EXPECT_EQ(flagForType<int8_t>(), MatchFlags::B8);
    EXPECT_EQ(flagForType<uint8_t>(), MatchFlags::B8);
    EXPECT_EQ(flagForType<int16_t>(), MatchFlags::B16);
    EXPECT_EQ(flagForType<uint16_t>(), MatchFlags::B16);
    EXPECT_EQ(flagForType<int32_t>(), MatchFlags::B32);
    EXPECT_EQ(flagForType<uint32_t>(), MatchFlags::B32);
    EXPECT_EQ(flagForType<int64_t>(), MatchFlags::B64);
    EXPECT_EQ(flagForType<uint64_t>(), MatchFlags::B64);
    EXPECT_EQ(flagForType<float>(), MatchFlags::B32);
    EXPECT_EQ(flagForType<double>(), MatchFlags::B64);
}

TEST(ReadHelpersTest, ReadTyped_Int32) {
    std::array<uint8_t, 8> buffer{};
    int32_t testValue = 0x12345678;
    std::memcpy(buffer.data(), &testValue, sizeof(testValue));

    Mem64 mem{buffer.data(), buffer.size()};
    auto result = readTyped<int32_t>(&mem, buffer.size(), false);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, testValue);
}

TEST(ReadHelpersTest, ReadTyped_InsufficientLength) {
    std::array<uint8_t, 2> buffer{};
    Mem64 mem{buffer.data(), buffer.size()};

    // Try to read 4 bytes from a 2-byte buffer
    auto result = readTyped<int32_t>(&mem, buffer.size(), false);

    EXPECT_FALSE(result.has_value());
}

TEST(ReadHelpersTest, UserValueAs) {
    UserValue uv = makeUserValue<int32_t>(42);

    EXPECT_EQ(userValueAs<int32_t>(uv), 42);
}

TEST(ReadHelpersTest, AlmostEqual_Float) {
    EXPECT_TRUE(almostEqual<float>(1.0F, 1.0F));
    EXPECT_TRUE(almostEqual<float>(1.0F, 1.0000001F));
    EXPECT_FALSE(almostEqual<float>(1.0F, 1.1F));
}

TEST(ReadHelpersTest, AlmostEqual_Double) {
    EXPECT_TRUE(almostEqual<double>(1.0, 1.0));
    EXPECT_TRUE(almostEqual<double>(1.0, 1.0000000000001));
    EXPECT_FALSE(almostEqual<double>(1.0, 1.001));
}

// =============================================================================
// Mem64 Tests
// =============================================================================

TEST(Mem64Test, TryGet_Int32) {
    std::array<uint8_t, 8> buffer{};
    int32_t testValue = 0xDEADBEEF;
    std::memcpy(buffer.data(), &testValue, sizeof(testValue));

    Mem64 mem{buffer.data(), buffer.size()};
    auto result = mem.tryGet<int32_t>();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, testValue);
}

TEST(Mem64Test, TryGet_InsufficientSize) {
    std::array<uint8_t, 2> buffer{0x42, 0x43};

    Mem64 mem{buffer.data(), buffer.size()};
    auto result = mem.tryGet<int32_t>();

    EXPECT_FALSE(result.has_value());
}

// =============================================================================
// Scan Type Utility Tests
// =============================================================================

TEST(ScanTypesTest, IsNumericType) {
    EXPECT_TRUE(isNumericType(ScanDataType::INTEGER8));
    EXPECT_TRUE(isNumericType(ScanDataType::INTEGER16));
    EXPECT_TRUE(isNumericType(ScanDataType::INTEGER32));
    EXPECT_TRUE(isNumericType(ScanDataType::INTEGER64));
    EXPECT_TRUE(isNumericType(ScanDataType::FLOAT32));
    EXPECT_TRUE(isNumericType(ScanDataType::FLOAT64));
    EXPECT_TRUE(isNumericType(ScanDataType::ANYINTEGER));
    EXPECT_TRUE(isNumericType(ScanDataType::ANYFLOAT));
    EXPECT_TRUE(isNumericType(ScanDataType::ANYNUMBER));
    EXPECT_FALSE(isNumericType(ScanDataType::BYTEARRAY));
    EXPECT_FALSE(isNumericType(ScanDataType::STRING));
}

TEST(ScanTypesTest, IsAggregatedAny) {
    EXPECT_TRUE(isAggregatedAny(ScanDataType::ANYNUMBER));
    EXPECT_TRUE(isAggregatedAny(ScanDataType::ANYINTEGER));
    EXPECT_TRUE(isAggregatedAny(ScanDataType::ANYFLOAT));
    EXPECT_FALSE(isAggregatedAny(ScanDataType::INTEGER32));
    EXPECT_FALSE(isAggregatedAny(ScanDataType::STRING));
}

TEST(ScanTypesTest, MatchNeedsUserValue) {
    EXPECT_TRUE(matchNeedsUserValue(ScanMatchType::MATCHEQUALTO));
    EXPECT_TRUE(matchNeedsUserValue(ScanMatchType::MATCHNOTEQUALTO));
    EXPECT_TRUE(matchNeedsUserValue(ScanMatchType::MATCHGREATERTHAN));
    EXPECT_TRUE(matchNeedsUserValue(ScanMatchType::MATCHLESSTHAN));
    EXPECT_TRUE(matchNeedsUserValue(ScanMatchType::MATCHRANGE));
    EXPECT_FALSE(matchNeedsUserValue(ScanMatchType::MATCHANY));
    EXPECT_FALSE(matchNeedsUserValue(ScanMatchType::MATCHCHANGED));
    EXPECT_FALSE(matchNeedsUserValue(ScanMatchType::MATCHINCREASED));
}

TEST(ScanTypesTest, MatchUsesOldValue) {
    EXPECT_TRUE(matchUsesOldValue(ScanMatchType::MATCHUPDATE));
    EXPECT_TRUE(matchUsesOldValue(ScanMatchType::MATCHNOTCHANGED));
    EXPECT_TRUE(matchUsesOldValue(ScanMatchType::MATCHCHANGED));
    EXPECT_TRUE(matchUsesOldValue(ScanMatchType::MATCHINCREASED));
    EXPECT_TRUE(matchUsesOldValue(ScanMatchType::MATCHDECREASED));
    EXPECT_FALSE(matchUsesOldValue(ScanMatchType::MATCHANY));
    EXPECT_FALSE(matchUsesOldValue(ScanMatchType::MATCHEQUALTO));
}
