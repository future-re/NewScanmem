// Unit tests for scan.string module - focus on nullptr safety

#include <gtest/gtest.h>

#include <cstdint>
#include <span>
#include <string>
#include <vector>

import scan.string;
import scan.routine;
import scan.types;
import value.core;
import value.flags;

class ScanStringTest : public ::testing::Test {
   protected:
    Value m_mem;
};

TEST_F(ScanStringTest,
       MakeStringScanRoutineMATCH_ANYMatchesFullLength) {
    std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o'};
    m_mem = Value(data);

    auto routine = makeStringScanRoutine(ScanMatchType::MATCH_ANY);
    auto ctx = scan::makeScanContext(
        std::span<const uint8_t>(m_mem.bytes.data(), m_mem.bytes.size()),
        nullptr, nullptr, MatchFlags::EMPTY, false);
    auto result = routine(ctx);

    EXPECT_EQ(result.matchLength, data.size());
}

TEST_F(ScanStringTest,
       MakeStringScanRoutineStringMatchWorks) {
    std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o', ' ',
                                 'W', 'o', 'r', 'l', 'd'};
    m_mem = Value(data);

    UserValue userValue = UserValue::fromString("Hello");

    auto routine = makeStringScanRoutine(ScanMatchType::MATCH_EQUAL_TO);
    auto ctx = scan::makeScanContext(
        std::span<const uint8_t>(m_mem.bytes.data(), m_mem.bytes.size()),
        nullptr, &userValue, userValue.flag(), false);

    auto result = routine(ctx);

    EXPECT_EQ(result.matchLength, 5U);  // "Hello" is 5 bytes
}

TEST_F(ScanStringTest, RegexMatchWithNullSaveFlagsDoesNotCrash) {
    std::vector<uint8_t> data = {'t', 'e', 's', 't', '1', '2', '3'};
    m_mem = Value(data);

    UserValue userValue = UserValue::fromString("[0-9]+");

    auto routine = makeStringScanRoutine(ScanMatchType::MATCH_REGEX);
    auto ctx = scan::makeScanContext(
        std::span<const uint8_t>(m_mem.bytes.data(), m_mem.bytes.size()),
        nullptr, &userValue, userValue.flag(), false);

    auto result = routine(ctx);

    EXPECT_EQ(result.matchLength, 3U);  // "123" matches the regex
}

TEST_F(ScanStringTest, StringRoutineSetsFlags) {
    std::vector<uint8_t> data = {'T', 'e', 's', 't'};
    m_mem = Value(data);

    auto routine = makeStringScanRoutine(ScanMatchType::MATCH_ANY);
    auto ctx = scan::makeScanContext(
        std::span<const uint8_t>(m_mem.bytes.data(), m_mem.bytes.size()),
        nullptr, nullptr, MatchFlags::EMPTY, false);
    auto result = routine(ctx);

    EXPECT_EQ(result.matchLength, data.size());
    EXPECT_EQ(result.matchedFlag, MatchFlags::B8);
}

// Test findRegexPattern directly
TEST_F(ScanStringTest, FindRegexPatternReturnsMatch) {
    std::vector<uint8_t> data = {'a', 'b', 'c', '1', '2', '3', 'x', 'y', 'z'};
    m_mem = Value(data);

    auto match = findRegexPattern(&m_mem, m_mem.bytes.size(), "[0-9]+");

    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->offset, 3U);  // "123" starts at offset 3
    EXPECT_EQ(match->length, 3U);  // "123" has length 3
}

// Test getCachedRegex with invalid pattern
TEST_F(ScanStringTest, GetCachedRegexInvalidPatternReturnsNull) {
    const auto* regex = getCachedRegex("[invalid(");

    EXPECT_EQ(regex, nullptr);
}
