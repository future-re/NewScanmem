#include <gtest/gtest.h>

#include <cstdint>

import utils.endianness;

class EndiannessTest : public ::testing::Test {
   protected:
    void SetUp() override {
        // 在每个测试之前执行的设置代码
    }

    void TearDown() override {
        // 在每个测试之后执行的清理代码
    }
};

// 测试 utils::swapBytesIntegral 函数
TEST_F(EndiannessTest, SwapBytesIntegral) {
    // 测试 uint8_t（1 字节，无变化）
    EXPECT_EQ(utils::swapBytesIntegral<uint8_t>(0xAB), 0xAB);
    EXPECT_EQ(utils::swapBytesIntegral<uint8_t>(0x00), 0x00);
    EXPECT_EQ(utils::swapBytesIntegral<uint8_t>(0xFF), 0xFF);

    // 测试 uint16_t（2 字节）
    EXPECT_EQ(utils::swapBytesIntegral<uint16_t>(0xABCD), 0xCDAB);
    EXPECT_EQ(utils::swapBytesIntegral<uint16_t>(0x0000), 0x0000);
    EXPECT_EQ(utils::swapBytesIntegral<uint16_t>(0xFFFF), 0xFFFF);
    EXPECT_EQ(utils::swapBytesIntegral<uint16_t>(0x1234), 0x3412);

    // 测试 uint32_t（4 字节）
    EXPECT_EQ(utils::swapBytesIntegral<uint32_t>(0xABCD1234), 0x3412CDAB);
    EXPECT_EQ(utils::swapBytesIntegral<uint32_t>(0x00000000), 0x00000000);
    EXPECT_EQ(utils::swapBytesIntegral<uint32_t>(0xFFFFFFFF), 0xFFFFFFFF);
    EXPECT_EQ(utils::swapBytesIntegral<uint32_t>(0x12345678), 0x78563412);

    // 测试 uint64_t（8 字节）
    EXPECT_EQ(utils::swapBytesIntegral<uint64_t>(0xABCD1234567890EF),
              0xEF9078563412CDAB);
    EXPECT_EQ(utils::swapBytesIntegral<uint64_t>(0x0000000000000000),
              0x0000000000000000);
    EXPECT_EQ(utils::swapBytesIntegral<uint64_t>(0xFFFFFFFFFFFFFFFF),
              0xFFFFFFFFFFFFFFFF);

    // 测试 round-trip（双重交换恢复原值）
    uint16_t val16 = 0x1234;
    EXPECT_EQ(utils::swapBytesIntegral(utils::swapBytesIntegral(val16)), val16);

    uint32_t val32 = 0x12345678;
    EXPECT_EQ(utils::swapBytesIntegral(utils::swapBytesIntegral(val32)), val32);

    uint64_t val64 = 0x123456789ABCDEF0;
    EXPECT_EQ(utils::swapBytesIntegral(utils::swapBytesIntegral(val64)), val64);
}

// 测试 utils::hostToNetwork 和 utils::networkToHost 函数
TEST_F(EndiannessTest, HostToNetworkAndNetworkToHost) {
    uint16_t val16 = 0x1234;
    uint32_t val32 = 0x12345678;
    uint64_t val64 = 0x123456789ABCDEF0;

    // 测试 round-trip（假设主机字节序）
    EXPECT_EQ(utils::networkToHost(utils::hostToNetwork(val16)), val16);
    EXPECT_EQ(utils::networkToHost(utils::hostToNetwork(val32)), val32);
    EXPECT_EQ(utils::networkToHost(utils::hostToNetwork(val64)), val64);

    // 测试典型值（行为取决于主机字节序）
    if constexpr (utils::isBigEndian()) {
        EXPECT_EQ(utils::hostToNetwork(val16), val16);  // 无变化
        EXPECT_EQ(utils::networkToHost(val16), val16);
    } else {
        EXPECT_EQ(utils::hostToNetwork(val16), utils::swapBytesIntegral(val16));  // 交换
        EXPECT_EQ(utils::networkToHost(val16), utils::swapBytesIntegral(val16));
    }
}

// 测试 utils::hostToLittleEndian 和 utils::littleEndianToHost 函数
TEST_F(EndiannessTest, HostToLittleEndianAndLittleEndianToHost) {
    uint16_t val16 = 0x1234;
    uint32_t val32 = 0x12345678;
    uint64_t val64 = 0x123456789ABCDEF0;

    // 测试 round-trip
    EXPECT_EQ(utils::littleEndianToHost(utils::hostToLittleEndian(val16)), val16);
    EXPECT_EQ(utils::littleEndianToHost(utils::hostToLittleEndian(val32)), val32);
    EXPECT_EQ(utils::littleEndianToHost(utils::hostToLittleEndian(val64)), val64);

    // 测试典型值
    if constexpr (utils::isLittleEndian()) {
        EXPECT_EQ(utils::hostToLittleEndian(val16), val16);  // 无变化
        EXPECT_EQ(utils::littleEndianToHost(val16), val16);
    } else {
        EXPECT_EQ(utils::hostToLittleEndian(val16), utils::swapBytesIntegral(val16));  // 交换
        EXPECT_EQ(utils::littleEndianToHost(val16), utils::swapBytesIntegral(val16));
    }
}