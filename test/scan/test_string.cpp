// Unit tests for scan.string module - focus on nullptr safety

import scan.string;
import scan.types;
import utils.mem64;
import value;
import value.flags;

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

class ScanStringTest : public ::testing::Test {
   protected:
    Mem64 m_mem;
};

// Test makeStringRoutine MATCH_ANY with nullptr saveFlags
TEST_F(ScanStringTest,
       MakeStringRoutineMATCH_ANYWithNullSaveFlagsDoesNotCrash) {
    std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o'};
    m_mem = Mem64(data);

    auto routine = makeStringRoutine(ScanMatchType::MATCH_ANY);

    // Should not crash with nullptr saveFlags
    unsigned int result =
        routine(&m_mem, m_mem.bytes().size(), nullptr, nullptr, nullptr);

    EXPECT_EQ(result, static_cast<unsigned int>(data.size()));
}

// Test makeStringRoutine with string match and nullptr saveFlags
TEST_F(ScanStringTest,
       MakeStringRoutineStringMatchWithNullSaveFlagsDoesNotCrash) {
    std::vector<uint8_t> data = {'H', 'e', 'l', 'l', 'o', ' ',
                                 'W', 'o', 'r', 'l', 'd'};
    m_mem = Mem64(data);

    UserValue userValue;
    userValue.stringValue = "Hello";
    userValue.flags = MatchFlags::STRING;

    auto routine = makeStringRoutine(ScanMatchType::MATCH_EQUAL_TO);

    // Should not crash with nullptr saveFlags
    unsigned int result =
        routine(&m_mem, m_mem.bytes().size(), nullptr, &userValue, nullptr);

    EXPECT_EQ(result, 5U);  // "Hello" is 5 bytes
}

// Test regex match with nullptr saveFlags
TEST_F(ScanStringTest, RegexMatchWithNullSaveFlagsDoesNotCrash) {
    std::vector<uint8_t> data = {'t', 'e', 's', 't', '1', '2', '3'};
    m_mem = Mem64(data);

    UserValue userValue;
    userValue.stringValue = "[0-9]+";
    userValue.flags = MatchFlags::STRING;

    auto routine = makeStringRoutine(ScanMatchType::MATCH_REGEX);

    // Should not crash with nullptr saveFlags
    unsigned int result =
        routine(&m_mem, m_mem.bytes().size(), nullptr, &userValue, nullptr);

    EXPECT_EQ(result, 3U);  // "123" matches the regex
}

// Verify that when saveFlags is provided, it's correctly set
TEST_F(ScanStringTest, StringRoutineSetsFlags) {
    std::vector<uint8_t> data = {'T', 'e', 's', 't'};
    m_mem = Mem64(data);

    auto routine = makeStringRoutine(ScanMatchType::MATCH_ANY);
    MatchFlags flags = MatchFlags::EMPTY;

    unsigned int result =
        routine(&m_mem, m_mem.bytes().size(), nullptr, nullptr, &flags);

    EXPECT_EQ(result, static_cast<unsigned int>(data.size()));
    EXPECT_EQ(flags, MatchFlags::B8);
}

// Test findRegexPattern directly
TEST_F(ScanStringTest, FindRegexPatternReturnsMatch) {
    std::vector<uint8_t> data = {'a', 'b', 'c', '1', '2', '3', 'x', 'y', 'z'};
    m_mem = Mem64(data);

    auto match = findRegexPattern(&m_mem, m_mem.bytes().size(), "[0-9]+");

    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->offset, 3U);  // "123" starts at offset 3
    EXPECT_EQ(match->length, 3U);  // "123" has length 3
}

// Test getCachedRegex with invalid pattern
TEST_F(ScanStringTest, GetCachedRegexInvalidPatternReturnsNull) {
    const auto* regex = getCachedRegex("[invalid(");

    EXPECT_EQ(regex, nullptr);
}
