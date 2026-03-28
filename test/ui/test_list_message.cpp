#include <gtest/gtest.h>

#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

import core.match;
import core.match_formatter;
import ui.show_message;
import scan.types;

using namespace core;

TEST(ListMessageTest, FormatVariousDataTypes) {
    // 1. Test Integer 32
    {
        std::vector<MatchEntry> entries;
        int32_t val = 12345678;
        std::vector<uint8_t> bytes(4);
        std::memcpy(bytes.data(), &val, 4);
        entries.push_back({0, 0x1000, bytes, "heap"});

        FormatOptions options;
        options.dataType = ScanDataType::INTEGER_32;
        options.showIndex = true;
        options.showRegion = true;

        auto lines = MatchFormatter::format(entries, 1, options);
        std::string out;
        for (const auto& line : lines) {
            out += line + "\n";
        }

        // 打印出来以便直接观察效果
        std::cout << out << std::endl;

        EXPECT_NE(out.find("12345678"), std::string::npos);
        EXPECT_NE(out.find("0x0000000000001000"), std::string::npos);
        EXPECT_NE(out.find("[heap]"), std::string::npos);
    }

    // 2. Test Float 32
    {
        std::vector<MatchEntry> entries;
        float val = 3.14159F;
        std::vector<uint8_t> bytes(4);
        std::memcpy(bytes.data(), &val, 4);
        entries.push_back({1, 0x2000, bytes, "stack"});

        FormatOptions options;
        options.dataType = ScanDataType::FLOAT_32;

        auto lines = MatchFormatter::format(entries, 1, options);
        std::string out;
        for (const auto& line : lines) {
            out += line + "\n";
        }
        std::cout << out << std::endl;

        EXPECT_NE(out.find("3.14159"), std::string::npos);
        EXPECT_NE(out.find("[stack]"), std::string::npos);
    }

    // 3. Test String
    {
        std::vector<MatchEntry> entries;
        std::string val = "NewScanmem";
        std::vector<uint8_t> bytes(val.begin(), val.end());
        entries.push_back({2, 0x3000, bytes, "anon"});

        FormatOptions options;
        options.dataType = ScanDataType::STRING;

        auto lines = MatchFormatter::format(entries, 1, options);
        std::string out;
        for (const auto& line : lines) {
            out += line + "\n";
        }
        std::cout << out << std::endl;

        EXPECT_NE(out.find("NewScanmem"), std::string::npos);
    }

    // 4. Test Hex Bytes (No data type)
    {
        std::vector<MatchEntry> entries;
        std::vector<uint8_t> bytes = {0xDE, 0xAD, 0xBE, 0xEF};
        entries.push_back({3, 0x4000, bytes, "code"});

        FormatOptions options;
        options.dataType = std::nullopt;

        auto lines = MatchFormatter::format(entries, 1, options);
        std::string out;
        for (const auto& line : lines) {
            out += line + "\n";
        }
        std::cout << out << std::endl;

        EXPECT_NE(out.find("0xde 0xad 0xbe 0xef"), std::string::npos);
    }
}

TEST(ListMessageTest, FormatOptions) {
    std::vector<MatchEntry> entries = {{.index = 0,
                                        .address = 0x1000,
                                        .value = {0x01, 0x00, 0x00, 0x00},
                                        .region = "region1"}};
    FormatOptions options;
    options.dataType = ScanDataType::INTEGER_32;

    // Test without index and region
    options.showIndex = false;
    options.showRegion = false;
    auto lines = MatchFormatter::format(entries, 1, options);
    std::string out;
    for (const auto& line : lines) {
        out += line + "\n";
    }
    EXPECT_EQ(out.find("Index"), std::string::npos);
    EXPECT_EQ(out.find("region1"), std::string::npos);

    // Test with summary for more matches
    lines = MatchFormatter::format(entries, 100, options);
    out.clear();
    for (const auto& line : lines) {
        out += line + "\n";
    }
    EXPECT_NE(out.find("and 99 more matches"), std::string::npos);
    EXPECT_NE(out.find("total: 100"), std::string::npos);
}
