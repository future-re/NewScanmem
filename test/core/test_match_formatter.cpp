// Unit tests for core::match_formatter
#include <gtest/gtest.h>

#include <cstring>
import core.match_formatter;
import core.match;
import scan.types;

using namespace core;

TEST(MatchFormatterTest, FormatEmptyValue) {
    std::vector<std::uint8_t> empty;
    auto result = formatValueByType(empty, ScanDataType::INTEGER_32, false);

    EXPECT_EQ(result, "0x00");
}

TEST(MatchFormatterTest, FormatHexBytes) {
    std::vector<std::uint8_t> bytes = {0x01, 0x02, 0x03};
    auto result = formatValueByType(bytes, std::nullopt, false);

    EXPECT_NE(result.find("0x01"), std::string::npos);
    EXPECT_NE(result.find("0x02"), std::string::npos);
    EXPECT_NE(result.find("0x03"), std::string::npos);
}

TEST(MatchFormatterTest, FormatInt32) {
    std::vector<std::uint8_t> bytes = {0x78, 0x56, 0x34,
                                       0x12};  // Little endian 0x12345678
    auto result = formatValueByType(bytes, ScanDataType::INTEGER_32, false);

    EXPECT_FALSE(result.empty());
}

TEST(MatchFormatterTest, FormatInt32BigEndian) {
    std::vector<std::uint8_t> bytes = {0x12, 0x34, 0x56, 0x78};
    auto result = formatValueByType(bytes, ScanDataType::INTEGER_32, true);

    EXPECT_FALSE(result.empty());
}

TEST(MatchFormatterTest, FormatFloat) {
    std::vector<std::uint8_t> bytes(sizeof(float));
    float testValue = 3.14f;
    std::memcpy(bytes.data(), &testValue, sizeof(float));

    auto result = formatValueByType(bytes, ScanDataType::FLOAT_32, false);

    EXPECT_FALSE(result.empty());
}

TEST(MatchFormatterTest, InsufficientBytes) {
    std::vector<std::uint8_t> bytes = {0x01};  // Only 1 byte for int32
    auto result = formatValueByType(bytes, ScanDataType::INTEGER_32, false);

    // Should handle gracefully
    EXPECT_FALSE(result.empty());
}
