#include <gtest/gtest.h>

import value.scalar;
import utils.endianness;

TEST(ScalarTest, BasicOperations) {
    ScalarValue sval = ScalarValue::make<uint32_t>(42);
    EXPECT_EQ(sval.kind, ScalarKind::U32);
    auto val = sval.get<uint32_t>();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 42U);
}

TEST(ScalarTest, ReadFromAddress) {
    uint32_t data = 0x12345678;
    auto svalOpt = ScalarValue::readFromAddress<uint32_t>(&data);
    ASSERT_TRUE(svalOpt.has_value());
    auto sval = *svalOpt;
    EXPECT_EQ(sval.kind, ScalarKind::U32);
    auto val = sval.get<uint32_t>();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 0x12345678U);
}

TEST(ScalarTest, FromBytes) {
    uint32_t data = 0x12345678;
    auto svalOpt = ScalarValue::fromBytes(
        ScalarKind::U32, reinterpret_cast<const uint8_t*>(&data), sizeof(data));
    ASSERT_TRUE(svalOpt.has_value());
    auto sval = *svalOpt;
    EXPECT_EQ(sval.kind, ScalarKind::U32);
    auto val = sval.get<uint32_t>();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 0x12345678U);
}
