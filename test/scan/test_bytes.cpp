import scan.bytes;
import scan.string;
import scan.types;
import utils.mem64;
import value;

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace {

TEST(ScanBytesTest, CompareBytesMatchesPrefix) {
    const std::vector<std::uint8_t> haystack{1, 2, 3, 4};
    Mem64 mem{haystack.data(), haystack.size()};
    const std::vector<std::uint8_t> needle{1, 2};
    MatchFlags flags = MatchFlags::EMPTY;
    const unsigned matched =
        compareBytes(&mem, haystack.size(), needle, &flags);
    EXPECT_EQ(matched, needle.size());
    EXPECT_NE(flags, MatchFlags::EMPTY);
}

TEST(ScanBytesTest, CompareBytesMaskedAllowsMaskedBits) {
    const std::vector<std::uint8_t> haystack{0xAA, 0xB5};
    Mem64 mem{haystack.data(), haystack.size()};
    const std::vector<std::uint8_t> needle{0xAA, 0xBB};
    const std::vector<std::uint8_t> mask{0xFF, 0xF0};  // low nibble ignored
    MatchFlags flags = MatchFlags::EMPTY;
    const unsigned matched =
        compareBytesMasked(&mem, haystack.size(), needle, mask, &flags);
    EXPECT_EQ(matched, needle.size());
    EXPECT_NE(flags, MatchFlags::EMPTY);
}

TEST(ScanBytesTest, FindBytePatternReturnsOffset) {
    const std::string text = "abcxabcd";
    Mem64 mem{text};
    const std::vector<std::uint8_t> needle{'a', 'b', 'c', 'd'};
    auto match = findBytePattern(&mem, mem.size(), needle);
    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->offset, 4U);
    EXPECT_EQ(match->length, needle.size());
}

TEST(ScanBytesTest, FindBytePatternMaskedIgnoresMaskedBits) {
    const std::vector<std::uint8_t> haystack{0x10, 0x20, 0x30};
    Mem64 mem{haystack.data(), haystack.size()};
    const std::vector<std::uint8_t> needle{0x00, 0x20};
    const std::vector<std::uint8_t> mask{0x00, 0xFF};  // first byte wildcard
    auto match = findBytePatternMasked(&mem, mem.size(), needle, mask);
    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->offset, 0U);
    EXPECT_EQ(match->length, needle.size());
}

TEST(ScanBytesTest, BytearrayRoutineWithMaskMatches) {
    UserValue userValue;
    userValue.bytearrayValue = std::vector<std::uint8_t>{0xAA, 0xBB};
    userValue.byteMask = std::vector<std::uint8_t>{0xFF, 0xF0};
    userValue.flags = MatchFlags::B8;

    auto routine = makeBytearrayRoutine(ScanMatchType::MATCH_EQUAL_TO);
    const std::vector<std::uint8_t> haystack{0xAA, 0xB5, 0x00};
    Mem64 mem{haystack.data(), haystack.size()};
    MatchFlags flags = MatchFlags::EMPTY;
    const unsigned matched =
        routine(&mem, haystack.size(), nullptr, &userValue, &flags);
    EXPECT_EQ(matched, 2U);
    EXPECT_NE(flags, MatchFlags::EMPTY);
}

TEST(ScanStringTest, MATCH_ANYReturnsFullLength) {
    const std::string text = "hello";
    Mem64 mem{text};
    auto routine = makeStringRoutine(ScanMatchType::MATCH_ANY);
    MatchFlags flags = MatchFlags::EMPTY;
    const unsigned matched =
        routine(&mem, mem.size(), nullptr, nullptr, &flags);
    EXPECT_EQ(matched, text.size());
    EXPECT_NE(flags, MatchFlags::EMPTY);
}

TEST(ScanStringTest, RegexMatchUsesPattern) {
    const std::string text = "zzabczz";
    Mem64 mem{text};
    UserValue userValue;
    userValue.stringValue = "a.c";

    auto routine = makeStringRoutine(ScanMatchType::MATCH_REGEX);
    MatchFlags flags = MatchFlags::EMPTY;
    const unsigned matched =
        routine(&mem, mem.size(), nullptr, &userValue, &flags);
    EXPECT_EQ(matched, 3U);
    EXPECT_NE(flags, MatchFlags::EMPTY);
}

}  // namespace
