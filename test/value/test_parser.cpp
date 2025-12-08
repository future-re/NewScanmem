#include <gtest/gtest.h>

import value.parser;
import value.flags;
import scan.types;

TEST(ParserTests, ParseInteger_ValidDecimal) {
    auto result = value::parseInteger<int32_t>("12345");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 12345);
}

TEST(ParserTests, ParseInteger_ValidHex) {
    auto result = value::parseInteger<int32_t>("0x1A3F");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 0x1A3F);
}

TEST(ParserTests, ParseInteger_InvalidInput) {
    auto result = value::parseInteger<int32_t>("invalid");
    EXPECT_FALSE(result.has_value());
}

TEST(ParserTests, ParseInteger_OutOfRange) {
    auto result = value::parseInteger<int8_t>("128");
    EXPECT_FALSE(result.has_value());
}

TEST(ParserTests, ParseDouble_ValidInput) {
    auto result = value::parseDouble("123.456");
    ASSERT_TRUE(result.has_value());
    EXPECT_DOUBLE_EQ(result.value(), 123.456);
}

TEST(ParserTests, ParseDouble_InvalidInput) {
    auto result = value::parseDouble("invalid");
    EXPECT_FALSE(result.has_value());
}

TEST(ParserTests, BuildUserValue_IntegerScalar) {
    std::vector<std::string> args = {"42"};
    auto result = value::buildUserValue(ScanDataType::INTEGER_32,
                                        ScanMatchType::MATCH_EQUAL_TO, args, 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->flags, MatchFlags::B32);
    EXPECT_EQ(result->s32, 42);
}

TEST(ParserTests, BuildUserValue_IntegerRange) {
    std::vector<std::string> args = {"10", "20"};
    auto result = value::buildUserValue(ScanDataType::INTEGER_32,
                                        ScanMatchType::MATCH_RANGE, args, 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->flags, MatchFlags::B32);
    EXPECT_EQ(result->s32, 10);
    EXPECT_EQ(result->s32h, 20);
}

TEST(ParserTests, BuildUserValue_ByteArray) {
    std::vector<std::string> args = {"0xDEADBEEF"};
    auto result = value::buildUserValue(ScanDataType::BYTE_ARRAY,
                                        ScanMatchType::MATCH_EQUAL_TO, args, 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->flags, MatchFlags::BYTE_ARRAY);
    ASSERT_TRUE(result->bytearrayValue.has_value());
    const auto& bytes = result->bytearrayValue.value();
    ASSERT_EQ(bytes.size(), 4);
    EXPECT_EQ(bytes[0], 0xDE);
    EXPECT_EQ(bytes[1], 0xAD);
    EXPECT_EQ(bytes[2], 0xBE);
    EXPECT_EQ(bytes[3], 0xEF);
}

TEST(ParserTests, BuildUserValue_String) {
    std::vector<std::string> args = {"test_string"};
    auto result = value::buildUserValue(ScanDataType::STRING,
                                        ScanMatchType::MATCH_EQUAL_TO, args, 0);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->stringValue, "test_string");
}
