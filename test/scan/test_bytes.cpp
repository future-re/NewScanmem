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
    const std::vector<std::uint8_t> HAY_STACK{1, 2, 3, 4};
    Mem64 mem{HAY_STACK.data(), HAY_STACK.size()};
    const std::vector<std::uint8_t> PATTERN{1, 2};
    MatchFlags flags = MatchFlags::EMPTY;
    const unsigned MATCHED = compareBytes(
        &mem, HAY_STACK.size(), PATTERN.data(), PATTERN.size(), &flags);
    EXPECT_EQ(MATCHED, PATTERN.size());
    EXPECT_NE(flags, MatchFlags::EMPTY);
}

TEST(ScanBytesTest, CompareBytesMaskedAllowsMaskedBits) {
    const std::vector<std::uint8_t> HAY_STACK{0xAA, 0xB5};
    Mem64 mem{HAY_STACK.data(), HAY_STACK.size()};
    const std::vector<std::uint8_t> PATTERN{0xAA, 0xBB};
    const std::vector<std::uint8_t> MASK{0xFF, 0xF0};  // low nibble ignored
    MatchFlags flags = MatchFlags::EMPTY;
    const unsigned MATCHED =
        compareBytesMasked(&mem, HAY_STACK.size(), PATTERN.data(),
                           PATTERN.size(), MASK.data(), MASK.size(), &flags);
    EXPECT_EQ(MATCHED, PATTERN.size());
    EXPECT_NE(flags, MatchFlags::EMPTY);
}

TEST(ScanBytesTest, FindBytePatternReturnsOffset) {
    const std::string TEXT = "abcxabcd";
    Mem64 mem{TEXT};
    const std::vector<std::uint8_t> PATTERN{'a', 'b', 'c', 'd'};
    auto match = findBytePattern(&mem, mem.size(), PATTERN);
    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->offset, 4U);
    EXPECT_EQ(match->length, PATTERN.size());
}

TEST(ScanBytesTest, FindBytePatternMaskedIgnoresMaskedBits) {
    const std::vector<std::uint8_t> HAY_STACK{0x10, 0x20, 0x30};
    Mem64 mem{HAY_STACK.data(), HAY_STACK.size()};
    const std::vector<std::uint8_t> PATTERN{0x00, 0x20};
    const std::vector<std::uint8_t> MASK{0x00, 0xFF};  // first byte wildcard
    auto match = findBytePatternMasked(&mem, mem.size(), PATTERN, MASK);
    ASSERT_TRUE(match.has_value());
    EXPECT_EQ(match->offset, 0U);
    EXPECT_EQ(match->length, PATTERN.size());
}

TEST(ScanBytesTest, BytearrayRoutineWithMaskMatches) {
    UserValue userValue;
    userValue.bytearrayValue = std::vector<std::uint8_t>{0xAA, 0xBB};
    userValue.byteMask = std::vector<std::uint8_t>{0xFF, 0xF0};
    userValue.flags = MatchFlags::B8;

    auto routine = makeBytearrayRoutine(ScanMatchType::MATCH_EQUAL_TO);
    const std::vector<std::uint8_t> HAY_STACK{0xAA, 0xB5, 0x00};
    Mem64 mem{HAY_STACK.data(), HAY_STACK.size()};
    MatchFlags flags = MatchFlags::EMPTY;
    const unsigned MATCHED =
        routine(&mem, HAY_STACK.size(), nullptr, &userValue, &flags);
    EXPECT_EQ(MATCHED, 2U);
    EXPECT_NE(flags, MatchFlags::EMPTY);
}

TEST(ScanStringTest, MATCH_ANYReturnsFullLength) {
    const std::string TEXT = "hello";
    Mem64 mem{TEXT};
    auto routine = makeStringRoutine(ScanMatchType::MATCH_ANY);
    MatchFlags flags = MatchFlags::EMPTY;
    const unsigned MATCHED =
        routine(&mem, mem.size(), nullptr, nullptr, &flags);
    EXPECT_EQ(MATCHED, TEXT.size());
    EXPECT_NE(flags, MatchFlags::EMPTY);
}

TEST(ScanStringTest, RegexMatchUsesPattern) {
    const std::string TEXT = "zzabczz";
    Mem64 mem{TEXT};
    UserValue userValue;
    userValue.stringValue = "a.c";

    auto routine = makeStringRoutine(ScanMatchType::MATCH_REGEX);
    MatchFlags flags = MatchFlags::EMPTY;
    const unsigned MATCHED =
        routine(&mem, mem.size(), nullptr, &userValue, &flags);
    EXPECT_EQ(MATCHED, 3U);
    EXPECT_NE(flags, MatchFlags::EMPTY);
}

}  // namespace
