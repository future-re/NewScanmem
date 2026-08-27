#include <gtest/gtest.h>

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "newscanmem/scan/bytes.hpp"
#include "newscanmem/scan/routine.hpp"
#include "newscanmem/scan/string.hpp"
#include "newscanmem/scan/types.hpp"
#include "newscanmem/value/core.hpp"
#include "newscanmem/value/flags.hpp"

namespace {

template <typename T>
auto view(const std::vector<T>& values) -> std::span<const T> {
    return {values.data(), values.size()};
}

TEST(ScanBytesTest, CompareBytesMatchesPrefix) {
    const std::vector<std::uint8_t> haystack{1, 2, 3, 4};
    const std::vector<std::uint8_t> pattern{1, 2};
    MatchFlags flags = MatchFlags::EMPTY;

    const auto matched = compareBytes(view(haystack), view(pattern), &flags);

    EXPECT_EQ(matched, pattern.size());
    EXPECT_NE(flags, MatchFlags::EMPTY);
}

TEST(ScanBytesTest, CompareBytesMaskedAllowsMaskedBits) {
    const std::vector<std::uint8_t> haystack{0xAA, 0xB5};
    const std::vector<std::uint8_t> pattern{0xAA, 0xBB};
    const std::vector<std::uint8_t> mask{0xFF, 0xF0};
    MatchFlags flags = MatchFlags::EMPTY;

    const auto matched =
        compareBytesMasked(view(haystack), view(pattern), view(mask), &flags);

    EXPECT_EQ(matched, pattern.size());
    EXPECT_NE(flags, MatchFlags::EMPTY);
}

TEST(ScanBytesTest, EmptyPatternReturnsZero) {
    const std::vector<std::uint8_t> haystack{1, 2, 3};
    const std::vector<std::uint8_t> pattern{};
    MatchFlags flags = MatchFlags::EMPTY;

    EXPECT_EQ(compareBytes(view(haystack), view(pattern), &flags), 0U);
    EXPECT_EQ(flags, MatchFlags::EMPTY);
}

TEST(ScanBytesTest, PatternLongerThanHaystackReturnsZero) {
    const std::vector<std::uint8_t> haystack{1, 2};
    const std::vector<std::uint8_t> pattern{1, 2, 3};
    MatchFlags flags = MatchFlags::EMPTY;

    EXPECT_EQ(compareBytes(view(haystack), view(pattern), &flags), 0U);
    EXPECT_EQ(flags, MatchFlags::EMPTY);
}

TEST(ScanBytesTest, WildcardMaskMatches) {
    const std::vector<std::uint8_t> haystack{0xAA, 0x55};
    const std::vector<std::uint8_t> pattern{0x00, 0x00};
    const std::vector<std::uint8_t> mask{0x00, 0x00};
    MatchFlags flags = MatchFlags::EMPTY;

    const auto matched =
        compareBytesMasked(view(haystack), view(pattern), view(mask), &flags);

    EXPECT_EQ(matched, pattern.size());
    EXPECT_TRUE((flags & MatchFlags::BYTE_ARRAY) == MatchFlags::BYTE_ARRAY);
}

TEST(ScanBytesTest, NullFlagsPointerIsAllowed) {
    const std::vector<std::uint8_t> haystack{1, 2, 3};
    const std::vector<std::uint8_t> pattern{1, 2};

    EXPECT_EQ(compareBytes(view(haystack), view(pattern), nullptr),
              pattern.size());
}

TEST(ScanBytesTest, MaskSizeMismatchReturnsZero) {
    const std::vector<std::uint8_t> haystack{0xAA, 0x55};
    const std::vector<std::uint8_t> pattern{0xAA, 0x55};
    const std::vector<std::uint8_t> mask{0xFF};
    MatchFlags flags = MatchFlags::EMPTY;

    EXPECT_EQ(compareBytesMasked(view(haystack), view(pattern), view(mask),
                                 &flags),
              0U);
    EXPECT_EQ(flags, MatchFlags::EMPTY);
}

TEST(ScanBytesTest, FindBytePatternReturnsOffset) {
    const std::string text = "abcxabcd";
    const auto memory = std::span<const std::uint8_t>{
        reinterpret_cast<const std::uint8_t*>(text.data()), text.size()};
    const std::vector<std::uint8_t> pattern{'a', 'b', 'c', 'd'};

    const auto match = findBytePattern(memory, view(pattern));

    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->offset, 4U);
    EXPECT_EQ(match->length, pattern.size());
}

TEST(ScanBytesTest, FindBytePatternMaskedIgnoresMaskedBits) {
    const std::vector<std::uint8_t> haystack{0x10, 0x20, 0x30};
    const std::vector<std::uint8_t> pattern{0x00, 0x20};
    const std::vector<std::uint8_t> mask{0x00, 0xFF};

    const auto match =
        findBytePatternMasked(view(haystack), view(pattern), view(mask));

    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->offset, 0U);
    EXPECT_EQ(match->length, pattern.size());
}

TEST(ScanBytesTest, BytearrayRoutineWithMaskMatches) {
    UserValue userValue =
        UserValue::fromByteArray(std::vector<std::uint8_t>{0xAA, 0xBB});
    userValue.primary.mask = std::vector<std::uint8_t>{0xFF, 0xF0};
    const std::vector<std::uint8_t> haystack{0xAA, 0xB5, 0x00};

    auto routine = makeBytearrayScanRoutine(ScanMatchType::MATCH_EQUAL_TO);
    auto context = scan::makeScanContext(view(haystack), nullptr, &userValue,
                                         userValue.flag(), false);
    const auto result = routine(context);

    EXPECT_EQ(result.matchLength, 2U);
    EXPECT_TRUE((result.matchedFlag & MatchFlags::BYTE_ARRAY) ==
                MatchFlags::BYTE_ARRAY);
}

TEST(ScanStringTest, MatchAnyReturnsFullLength) {
    const std::string text = "hello";
    const auto memory = std::span<const std::uint8_t>{
        reinterpret_cast<const std::uint8_t*>(text.data()), text.size()};
    auto routine = makeStringScanRoutine(ScanMatchType::MATCH_ANY);
    auto context = scan::makeScanContext(memory, nullptr, nullptr,
                                         MatchFlags::EMPTY, false);

    const auto result = routine(context);

    EXPECT_EQ(result.matchLength, text.size());
    EXPECT_NE(result.matchedFlag, MatchFlags::EMPTY);
}

TEST(ScanStringTest, RegexBlockMatcherUsesPattern) {
    const std::string text = "zzabczz";
    const auto memory = std::span<const std::uint8_t>{
        reinterpret_cast<const std::uint8_t*>(text.data()), text.size()};

    const auto matches = findRegexMatches(memory, "a.c");

    ASSERT_EQ(matches.size(), 1U);
    EXPECT_EQ(matches[0].offset, 2U);
    EXPECT_EQ(matches[0].length, 3U);
}

}  // namespace
