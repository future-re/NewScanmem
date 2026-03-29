#include <gtest/gtest.h>

#include <span>
#include <string>
#include <vector>

import scan.bytes;
import scan.routine;
import scan.string;
import scan.types;
import value.core;
import value.flags;

namespace {

TEST(ScanBytesTest, CompareBytesMatchesPrefix) {
    const std::vector<std::uint8_t> HAY_STACK{1, 2, 3, 4};
    Value mem{HAY_STACK.data(), HAY_STACK.size()};
    const std::vector<std::uint8_t> PATTERN{1, 2};
    MatchFlags flags = MatchFlags::EMPTY;
    const unsigned MATCHED = compareBytes(
        &mem, HAY_STACK.size(), PATTERN.data(), PATTERN.size(), &flags);
    EXPECT_EQ(MATCHED, PATTERN.size());
    EXPECT_NE(flags, MatchFlags::EMPTY);
}

TEST(ScanBytesTest, CompareBytesMaskedAllowsMaskedBits) {
    const std::vector<std::uint8_t> HAY_STACK{0xAA, 0xB5};
    Value mem{HAY_STACK.data(), HAY_STACK.size()};
    const std::vector<std::uint8_t> PATTERN{0xAA, 0xBB};
    const std::vector<std::uint8_t> MASK{0xFF, 0xF0};  // low nibble ignored
    MatchFlags flags = MatchFlags::EMPTY;
    const unsigned MATCHED =
        compareBytesMasked(&mem, HAY_STACK.size(), PATTERN.data(),
                           PATTERN.size(), MASK.data(), MASK.size(), &flags);
    EXPECT_EQ(MATCHED, PATTERN.size());
    EXPECT_NE(flags, MatchFlags::EMPTY);
}

TEST(ScanBytesTest, CompareBytesEmptyPatternReturnsZero) {
    const std::vector<std::uint8_t> HAY_STACK{1, 2, 3};
    Value mem{HAY_STACK.data(), HAY_STACK.size()};
    const std::vector<std::uint8_t> PATTERN{};
    MatchFlags flags = MatchFlags::EMPTY;
    const unsigned MATCHED = compareBytes(
        &mem, HAY_STACK.size(), PATTERN.data(), PATTERN.size(), &flags);
    EXPECT_EQ(MATCHED, 0U);
    EXPECT_EQ(flags, MatchFlags::EMPTY);
}

TEST(ScanBytesTest, CompareBytesPatternLongerThanHaystack) {
    const std::vector<std::uint8_t> HAY_STACK{1, 2};
    Value mem{HAY_STACK.data(), HAY_STACK.size()};
    const std::vector<std::uint8_t> PATTERN{1, 2, 3};
    MatchFlags flags = MatchFlags::EMPTY;
    const unsigned MATCHED = compareBytes(
        &mem, HAY_STACK.size(), PATTERN.data(), PATTERN.size(), &flags);
    EXPECT_EQ(MATCHED, 0U);
    EXPECT_EQ(flags, MatchFlags::EMPTY);
}

TEST(ScanBytesTest, CompareBytesVectorOverloadMatches) {
    const std::vector<std::uint8_t> HAY_STACK{5, 6, 7, 8};
    Value mem{HAY_STACK.data(), HAY_STACK.size()};
    const std::vector<std::uint8_t> PATTERN{5, 6};
    MatchFlags flags = MatchFlags::EMPTY;
    const unsigned MATCHED =
        compareBytes(&mem, HAY_STACK.size(), PATTERN, &flags);
    EXPECT_EQ(MATCHED, PATTERN.size());
    EXPECT_NE(flags, MatchFlags::EMPTY);
}

TEST(ScanBytesTest, CompareBytesMaskedWildcardMaskMatches) {
    const std::vector<std::uint8_t> HAY_STACK{0xAA, 0x55};
    Value mem{HAY_STACK.data(), HAY_STACK.size()};
    const std::vector<std::uint8_t> PATTERN{0x00, 0x00};
    const std::vector<std::uint8_t> MASK{0x00, 0x00};  // wildcard everything
    MatchFlags flags = MatchFlags::EMPTY;
    const unsigned MATCHED =
        compareBytesMasked(&mem, HAY_STACK.size(), PATTERN.data(),
                           PATTERN.size(), MASK.data(), MASK.size(), &flags);
    EXPECT_EQ(MATCHED, PATTERN.size());
    EXPECT_NE(flags, MatchFlags::EMPTY);
    EXPECT_TRUE((flags & MatchFlags::BYTE_ARRAY) == MatchFlags::BYTE_ARRAY);
}

TEST(ScanBytesTest, CompareBytesWithNullSaveFlagsDoesNotCrash) {
    const std::vector<std::uint8_t> HAY_STACK{1, 2, 3};
    Value mem{HAY_STACK.data(), HAY_STACK.size()};
    const std::vector<std::uint8_t> PATTERN{1, 2};
    const unsigned MATCHED = compareBytes(
        &mem, HAY_STACK.size(), PATTERN.data(), PATTERN.size(), nullptr);
    EXPECT_EQ(MATCHED, PATTERN.size());
}

TEST(ScanBytesTest, CompareBytesMaskedWithNullSaveFlagsDoesNotCrash) {
    const std::vector<std::uint8_t> HAY_STACK{0xAA, 0xB5};
    Value mem{HAY_STACK.data(), HAY_STACK.size()};
    const std::vector<std::uint8_t> PATTERN{0xAA, 0xBB};
    const std::vector<std::uint8_t> MASK{0xFF, 0xF0};
    const unsigned MATCHED =
        compareBytesMasked(&mem, HAY_STACK.size(), PATTERN.data(),
                           PATTERN.size(), MASK.data(), MASK.size(), nullptr);
    EXPECT_EQ(MATCHED, PATTERN.size());
}

TEST(ScanBytesTest, CompareBytesMaskedMaskSizeMismatchReturnsZero) {
    const std::vector<std::uint8_t> HAY_STACK{0xAA, 0x55};
    Value mem{HAY_STACK.data(), HAY_STACK.size()};
    const std::vector<std::uint8_t> PATTERN{0xAA, 0x55};
    const std::vector<std::uint8_t> MASK{0xFF};
    MatchFlags flags = MatchFlags::EMPTY;
    const unsigned MATCHED =
        compareBytesMasked(&mem, HAY_STACK.size(), PATTERN.data(),
                           PATTERN.size(), MASK.data(), MASK.size(), &flags);
    EXPECT_EQ(MATCHED, 0U);
    EXPECT_EQ(flags, MatchFlags::EMPTY);
}

TEST(ScanBytesTest, FindBytePatternReturnsOffset) {
    const std::string TEXT = "abcxabcd";
    Value mem{reinterpret_cast<const std::uint8_t*>(TEXT.data()), TEXT.size()};
    const std::vector<std::uint8_t> PATTERN{'a', 'b', 'c', 'd'};
    auto match = findBytePattern(&mem, mem.size(), PATTERN);
    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->offset, 4U);
    EXPECT_EQ(match->length, PATTERN.size());
}

TEST(ScanBytesTest, FindBytePatternMaskedIgnoresMaskedBits) {
    const std::vector<std::uint8_t> HAY_STACK{0x10, 0x20, 0x30};
    Value mem{HAY_STACK.data(), HAY_STACK.size()};
    const std::vector<std::uint8_t> PATTERN{0x00, 0x20};
    const std::vector<std::uint8_t> MASK{0x00, 0xFF};  // first byte wildcard
    auto match = findBytePatternMasked(&mem, mem.size(), PATTERN, MASK);
    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->offset, 0U);
    EXPECT_EQ(match->length, PATTERN.size());
}

TEST(ScanBytesTest, BytearrayRoutineWithMaskMatches) {
    UserValue userValue =
        UserValue::fromByteArray(std::vector<uint8_t>{0xAA, 0xBB});
    userValue.primary.mask = std::vector<std::uint8_t>{0xFF, 0xF0};

    auto routine = makeBytearrayScanRoutine(ScanMatchType::MATCH_EQUAL_TO);
    const std::vector<std::uint8_t> HAY_STACK{0xAA, 0xB5, 0x00};
    auto ctx = scan::makeScanContext(
        std::span<const uint8_t>(HAY_STACK.data(), HAY_STACK.size()), nullptr,
        &userValue, userValue.flag(), false);
    auto result = routine(ctx);
    EXPECT_EQ(result.matchLength, 2U);
    EXPECT_NE(result.matchedFlag, MatchFlags::EMPTY);
}

TEST(ScanBytesTest, MakeBytearrayRoutineAddsBYTE_ARRAYFlag) {
    UserValue userValue =
        UserValue::fromByteArray(std::vector<uint8_t>{0xAA, 0xBB});
    userValue.primary.mask = std::vector<std::uint8_t>{0xFF, 0xF0};

    auto routine = makeBytearrayScanRoutine(ScanMatchType::MATCH_EQUAL_TO);
    const std::vector<std::uint8_t> HAY_STACK{0xAA, 0xB5, 0x00};
    auto ctx = scan::makeScanContext(
        std::span<const uint8_t>(HAY_STACK.data(), HAY_STACK.size()), nullptr,
        &userValue, userValue.flag(), false);
    auto result = routine(ctx);
    EXPECT_EQ(result.matchLength, 2U);
    EXPECT_TRUE((result.matchedFlag & MatchFlags::BYTE_ARRAY) ==
                MatchFlags::BYTE_ARRAY);
}

TEST(ScanStringTest, MATCH_ANYReturnsFullLength) {
    const std::string TEXT = "hello";
    auto routine = makeStringScanRoutine(ScanMatchType::MATCH_ANY);
    auto ctx = scan::makeScanContext(
        std::span<const uint8_t>(
            reinterpret_cast<const std::uint8_t*>(TEXT.data()), TEXT.size()),
        nullptr, nullptr, MatchFlags::EMPTY, false);
    auto result = routine(ctx);
    EXPECT_EQ(result.matchLength, TEXT.size());
    EXPECT_NE(result.matchedFlag, MatchFlags::EMPTY);
}

TEST(ScanStringTest, RegexMatchUsesPattern) {
    const std::string TEXT = "zzabczz";
    UserValue userValue = UserValue::fromString("a.c");

    auto routine = makeStringScanRoutine(ScanMatchType::MATCH_REGEX);
    auto ctx = scan::makeScanContext(
        std::span<const uint8_t>(
            reinterpret_cast<const std::uint8_t*>(TEXT.data()), TEXT.size()),
        nullptr, &userValue, userValue.flag(), false);
    auto result = routine(ctx);
    EXPECT_EQ(result.matchLength, 3U);
    EXPECT_NE(result.matchedFlag, MatchFlags::EMPTY);
}

}  // namespace
