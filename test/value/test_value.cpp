#include <gtest/gtest.h>

#include <string>
#include <vector>

import value;
import scan.types;

TEST(ValueTest, StringSupport) {
    auto val = UserValue::fromString("hello");
    EXPECT_EQ(val.flag(), MatchFlags::STRING);
    auto text = val.stringValue();
    ASSERT_TRUE(text.has_value());
    EXPECT_EQ(*text, "hello");

    // Test flagForType
    EXPECT_EQ(flagForType<std::string>(), MatchFlags::STRING);
}

TEST(ValueTest, ByteArraySupport) {
    std::vector<uint8_t> bytes = {0xDE, 0xAD, 0xBE, 0xEF};
    auto val = UserValue::fromByteArray(bytes);
    EXPECT_EQ(val.flag(), MatchFlags::BYTE_ARRAY);
    auto bytesOpt = val.byteArrayValue();
    ASSERT_TRUE(bytesOpt.has_value());
    std::vector<uint8_t> got(bytesOpt->begin(), bytesOpt->end());
    EXPECT_EQ(got, bytes);

    // Test flagForType
    EXPECT_EQ(flagForType<std::vector<uint8_t>>(),
              MatchFlags::BYTE_ARRAY);
}

TEST(ValueTest, RangeSupport) {
    UserValue val{Value::of<int32_t>(10), Value::of<int32_t>(20)};
    EXPECT_TRUE(val.hasSecondary());
    ASSERT_TRUE(val.secondary.has_value());
    EXPECT_EQ(val.primary.flag(), MatchFlags::B32);
    EXPECT_EQ(val.secondary->flag(), MatchFlags::B32);
}

TEST(ValueTest, ParserBuildsRangeAsPrimaryAndSecondary) {
    std::vector<std::string> args = {"10", "20"};
    auto value =
        value::buildUserValue(ScanDataType::INTEGER_32,
                              ScanMatchType::MATCH_RANGE, args, 0);
    ASSERT_TRUE(value.has_value());
    ASSERT_TRUE(value->secondary.has_value());
    EXPECT_EQ(value->primary.as<int32_t>().value_or(0), 10);
    EXPECT_EQ(value->secondary->as<int32_t>().value_or(0), 20);
}
