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

    if (std::endian::native == std::endian::little) {
        EXPECT_EQ(result, "305419896");  // 0x12345678
    } else {
        EXPECT_EQ(result, "2018915346");  // 0x78563412
    }
}

TEST(MatchFormatterTest, FormatInt32BigEndian) {
    std::vector<std::uint8_t> bytes = {0x12, 0x34, 0x56, 0x78};
    auto result = formatValueByType(bytes, ScanDataType::INTEGER_32, true);

    EXPECT_EQ(result, "305419896");  // 0x12345678
}

TEST(MatchFormatterTest, FormatInt8) {
    std::vector<std::uint8_t> bytes = {0x80};  // -128
    auto result = formatValueByType(bytes, ScanDataType::INTEGER_8, false);
    EXPECT_EQ(result, "-128");

    bytes = {0x7F};  // 127
    result = formatValueByType(bytes, ScanDataType::INTEGER_8, false);
    EXPECT_EQ(result, "127");
}

TEST(MatchFormatterTest, FormatInt16) {
    std::vector<std::uint8_t> bytes = {0x80,
                                       0x00};  // 0x8000 = -32768 in big endian
    auto result = formatValueByType(bytes, ScanDataType::INTEGER_16, true);
    EXPECT_EQ(result, "-32768");

    bytes = {0xFF, 0x7F};  // 0x7FFF = 32767 in little endian
    result = formatValueByType(bytes, ScanDataType::INTEGER_16, false);
    if (std::endian::native == std::endian::little) {
        EXPECT_EQ(result, "32767");
    }
}

TEST(MatchFormatterTest, FormatInt64) {
    std::vector<std::uint8_t> bytes = {0x80, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00};
    auto result = formatValueByType(bytes, ScanDataType::INTEGER_64, true);
    EXPECT_EQ(result, "-9223372036854775808");
}

TEST(MatchFormatterTest, FormatFloat) {
    std::vector<std::uint8_t> bytes(sizeof(float));
    float testValue = 3.14159F;
    std::memcpy(bytes.data(), &testValue, sizeof(float));

    auto result = formatValueByType(bytes, ScanDataType::FLOAT_32, false);
    EXPECT_EQ(result, "3.14159");
}

TEST(MatchFormatterTest, FormatDouble) {
    std::vector<std::uint8_t> bytes(sizeof(double));
    double testValue = 3.141592653589793;
    std::memcpy(bytes.data(), &testValue, sizeof(double));

    auto result = formatValueByType(bytes, ScanDataType::FLOAT_64, false);
    EXPECT_EQ(result, "3.14159265358979");
}

TEST(MatchFormatterTest, FormatString) {
    std::string testStr = "Hello World";
    std::vector<std::uint8_t> bytes(testStr.begin(), testStr.end());
    auto result = formatValueByType(bytes, ScanDataType::STRING, false);
    EXPECT_EQ(result, "Hello World");
}

TEST(MatchFormatterTest, DisplaySmokeTest) {
    std::vector<MatchEntry> entries = {{.index = 0,
                                        .address = 0x1000,
                                        .value = {0x01, 0x00, 0x00, 0x00},
                                        .region = "Region1"},
                                       {.index = 1,
                                        .address = 0x2000,
                                        .value = {0x02, 0x00, 0x00, 0x00},
                                        .region = "Region2"}};

    FormatOptions options;
    options.dataType = ScanDataType::INTEGER_32;

    // This just ensures it doesn't crash
    EXPECT_NO_THROW(MatchFormatter::display(entries, 2, options));
}

TEST(MatchFormatterTest, InsufficientBytes) {
    std::vector<std::uint8_t> bytes = {0x01};  // Only 1 byte for int32
    auto result = formatValueByType(bytes, ScanDataType::INTEGER_32, false);

    // Should handle gracefully
    EXPECT_FALSE(result.empty());
}
