// Unit tests for scan.numeric module - focus on nullptr safety

import scan.numeric;
import scan.types;
import utils.mem64;
import value;
import value.flags;

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

class ScanNumericTest : public ::testing::Test {
   protected:
    Mem64 m_mem;
};

// Test numericMatchCore with nullptr saveFlags
TEST_F(ScanNumericTest, NumericMatchCoreWithNullSaveFlagsDoesNotCrash) {
    std::vector<uint8_t> data = {42, 0, 0, 0};  // int32 = 42
    m_mem = Mem64(data);

    UserValue userValue = UserValue::fromScalar<int32_t>(42);

    // Should not crash with nullptr saveFlags
    unsigned int result = numericMatchCore<int32_t>(
        ScanMatchType::MATCH_EQUAL_TO, 42, nullptr, &userValue, nullptr);

    EXPECT_EQ(result, sizeof(int32_t));
}

// Test makeNumericRoutine with nullptr saveFlags
TEST_F(ScanNumericTest, MakeNumericRoutineWithNullSaveFlagsDoesNotCrash) {
    std::vector<uint8_t> data = {100, 0, 0, 0, 0, 0, 0, 0};  // int64 = 100
    m_mem = Mem64(data);

    auto routine = makeNumericRoutine<int64_t>(ScanMatchType::MATCH_ANY, false);

    // Should not crash with nullptr saveFlags
    unsigned int result =
        routine(&m_mem, m_mem.bytes().size(), nullptr, nullptr, nullptr);

    EXPECT_EQ(result, sizeof(int64_t));
}

// Test makeAnyIntegerRoutine with nullptr saveFlags
TEST_F(ScanNumericTest, MakeAnyIntegerRoutineWithNullSaveFlagsDoesNotCrash) {
    std::vector<uint8_t> data = {0xFF, 0x00};  // uint16 = 255
    m_mem = Mem64(data);

    auto routine = makeAnyIntegerRoutine(ScanMatchType::MATCH_ANY, false);

    // Should not crash with nullptr saveFlags
    unsigned int result =
        routine(&m_mem, m_mem.bytes().size(), nullptr, nullptr, nullptr);

    // Should match at least as uint8_t or larger
    EXPECT_GT(result, 0U);
}

// Test makeAnyFloatRoutine with nullptr saveFlags
TEST_F(ScanNumericTest, MakeAnyFloatRoutineWithNullSaveFlagsDoesNotCrash) {
    std::vector<uint8_t> data = {0, 0, 0x80, 0x3F};  // float = 1.0
    m_mem = Mem64(data);

    auto routine = makeAnyFloatRoutine(ScanMatchType::MATCH_ANY, false);

    // Should not crash with nullptr saveFlags
    unsigned int result =
        routine(&m_mem, m_mem.bytes().size(), nullptr, nullptr, nullptr);

    EXPECT_EQ(result, sizeof(float));
}

// Test makeAnyNumberRoutine with nullptr saveFlags
TEST_F(ScanNumericTest, MakeAnyNumberRoutineWithNullSaveFlagsDoesNotCrash) {
    std::vector<uint8_t> data = {42};  // uint8 = 42
    m_mem = Mem64(data);

    auto routine = makeAnyNumberRoutine(ScanMatchType::MATCH_ANY, false);

    // Should not crash with nullptr saveFlags
    unsigned int result =
        routine(&m_mem, m_mem.bytes().size(), nullptr, nullptr, nullptr);

    EXPECT_GT(result, 0U);
}

// Verify that when saveFlags is provided, it's correctly set
TEST_F(ScanNumericTest, NumericRoutineSetsFlags) {
    std::vector<uint8_t> data = {42, 0};  // uint16 = 42
    m_mem = Mem64(data);

    auto routine =
        makeNumericRoutine<uint16_t>(ScanMatchType::MATCH_ANY, false);
    MatchFlags flags = MatchFlags::EMPTY;

    unsigned int result =
        routine(&m_mem, m_mem.bytes().size(), nullptr, nullptr, &flags);

    EXPECT_EQ(result, sizeof(uint16_t));
    EXPECT_EQ(flags, MatchFlags::B16);
}
