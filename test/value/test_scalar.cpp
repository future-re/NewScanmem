#include <gtest/gtest.h>

import value.scalar;
import value.view;

import utils.endianness;

TEST(ScalarTest, BasicOperations) {
    ScalarValue sval = ScalarValue::make<uint32_t>(42);
    EXPECT_EQ(sval.kind, ScalarKind::U32);
    auto val = sval.get<uint32_t>();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 42U);
}

TEST(ScalarTest, ReadFromAddressBigEndian) {
    uint32_t data = 0x78563412;
    auto svalOpt = ScalarValue::readFromAddress<uint32_t>(&data, Endian::BIG);
    ASSERT_TRUE(svalOpt.has_value());
    auto sval = *svalOpt;
    EXPECT_EQ(sval.kind, ScalarKind::U32);
    auto val = sval.get<uint32_t>();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 0x12345678U);  // Converted to host endianness
}

TEST(ScalarTest, ReadFromAddressLittleEndian) {
    uint32_t data = 0x12345678;
    auto svalOpt =
        ScalarValue::readFromAddress<uint32_t>(&data, Endian::LITTLE);
    ASSERT_TRUE(svalOpt.has_value());
    auto sval = *svalOpt;
    EXPECT_EQ(sval.kind, ScalarKind::U32);
    auto val = sval.get<uint32_t>();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 0x12345678U);  // No conversion needed on little-endian host
}
