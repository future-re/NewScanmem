#include <gtest/gtest.h>

#include <string>
#include <vector>

import value.scalar;
import value.flags;

TEST(ValueTest, StringSupport) {
    auto val = UserValue::fromString("hello");
    EXPECT_EQ(val.flag(), MatchFlags::STRING);
    EXPECT_EQ(val.stringValue(), "hello");

    // Test flagForType
    EXPECT_EQ(flagForType<std::string>(), MatchFlags::STRING);
}

TEST(ValueTest, ByteArraySupport) {
    std::vector<uint8_t> bytes = {0xDE, 0xAD, 0xBE, 0xEF};
    auto val = UserValue::fromByteArray(bytes);
    EXPECT_EQ(val.flag(), MatchFlags::BYTE_ARRAY);
    EXPECT_EQ(val.byteArrayValue(), bytes);

    // Test flagForType
    EXPECT_EQ(flagForType<std::vector<uint8_t>>(),
              MatchFlags::BYTE_ARRAY);
}
