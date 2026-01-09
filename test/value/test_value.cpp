#include <gtest/gtest.h>

#include <string>
#include <vector>

import value;
import value.flags;

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
