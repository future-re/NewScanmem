#include <gtest/gtest.h>

#include <cstdint>
#include <span>
#include <string_view>

#include "memseek/scan/routine.hpp"
#include "memseek/scan/string.hpp"
#include "memseek/scan/types.hpp"
#include "memseek/value/core.hpp"
#include "memseek/value/flags.hpp"

namespace {

auto bytes(const std::string_view text) -> std::span<const std::uint8_t> {
    return {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()};
}

TEST(ScanStringTest, MatchAnyMatchesFullLength) {
    auto routine = makeStringScanRoutine(ScanMatchType::MATCH_ANY);
    auto context = scan::makeScanContext(bytes("Hello"), nullptr, nullptr,
                                         MatchFlags::EMPTY, false);

    const auto result = routine(context);

    EXPECT_TRUE(result);
    EXPECT_EQ(result.matchLength, 5U);
    EXPECT_EQ(result.matchedFlag, MatchFlags::B8);
}

TEST(ScanStringTest, ExactStringMatchWorks) {
    UserValue userValue = UserValue::fromString("Hello");
    auto routine = makeStringScanRoutine(ScanMatchType::MATCH_EQUAL_TO);
    auto context = scan::makeScanContext(bytes("Hello World"), nullptr,
                                         &userValue, userValue.flag(), false);

    const auto result = routine(context);

    EXPECT_TRUE(result);
    EXPECT_EQ(result.matchLength, 5U);
}

TEST(ScanStringTest, RegexBlockMatcherReturnsAllMatches) {
    const auto matches = findRegexMatches(bytes("abc123xyz456"), "[0-9]+");

    ASSERT_EQ(matches.size(), 2U);
    EXPECT_EQ(matches[0].offset, 3U);
    EXPECT_EQ(matches[0].length, 3U);
    EXPECT_EQ(matches[1].offset, 9U);
    EXPECT_EQ(matches[1].length, 3U);
}

TEST(ScanStringTest, RegexBlockMatcherPreservesOverlappingMatches) {
    const auto matches = findRegexMatches(bytes("ababa"), "aba");

    ASSERT_EQ(matches.size(), 2U);
    EXPECT_EQ(matches[0].offset, 0U);
    EXPECT_EQ(matches[0].length, 3U);
    EXPECT_EQ(matches[1].offset, 2U);
    EXPECT_EQ(matches[1].length, 3U);
}

TEST(ScanStringTest, InvalidRegexReturnsNoMatches) {
    EXPECT_TRUE(findRegexMatches(bytes("abc123"), "[invalid(").empty());
    EXPECT_EQ(getCachedRegex("[invalid("), nullptr);
}

TEST(ScanStringTest, RegexIsHandledByBlockMatcherNotPerAddressRoutine) {
    UserValue userValue = UserValue::fromString("a.c");
    auto routine = makeStringScanRoutine(ScanMatchType::MATCH_REGEX);
    auto context = scan::makeScanContext(bytes("abc"), nullptr, &userValue,
                                         userValue.flag(), false);

    EXPECT_FALSE(routine(context));
}

}  // namespace
