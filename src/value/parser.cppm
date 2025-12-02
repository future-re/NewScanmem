/**
 * @file parser.cppm
 * @brief Value parsing utilities for scan operations
 */

module;

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

export module value.parser;

import scan.types;
import value;
import value.flags;

export namespace value {

/**
 * @brief Convert string to lowercase
 */
inline auto toLower(std::string str) -> std::string {
    for (auto& character : str) {
        character = static_cast<char>(
            std::tolower(static_cast<unsigned char>(character)));
    }
    return str;
}

/**
 * @brief Parse data type from string with aliases support
 * @param tok Token to parse (e.g., "int", "int64", "float")
 * @return Optional ScanDataType
 */
[[nodiscard]] inline auto parseDataType(std::string_view tok)
    -> std::optional<ScanDataType> {
    const auto TO_STR = toLower(std::string(tok));
    if (TO_STR == "any" || TO_STR == "anynumber") {
        return ScanDataType::ANYNUMBER;
    }
    if (TO_STR == "anyint" || TO_STR == "anyinteger") {
        return ScanDataType::ANYINTEGER;
    }
    if (TO_STR == "anyfloat") {
        return ScanDataType::ANYFLOAT;
    }
    if (TO_STR == "int") {  // 常见别名：默认映射为 64 位整数
        return ScanDataType::INTEGER64;
    }
    if (TO_STR == "int8" || TO_STR == "i8") {
        return ScanDataType::INTEGER8;
    }
    if (TO_STR == "int16" || TO_STR == "i16") {
        return ScanDataType::INTEGER16;
    }
    if (TO_STR == "int32" || TO_STR == "i32") {
        return ScanDataType::INTEGER32;
    }
    if (TO_STR == "int64" || TO_STR == "i64") {
        return ScanDataType::INTEGER64;
    }
    if (TO_STR == "float" || TO_STR == "float32" || TO_STR == "f32") {
        return ScanDataType::FLOAT32;
    }
    if (TO_STR == "double" || TO_STR == "float64" || TO_STR == "f64") {
        return ScanDataType::FLOAT64;
    }
    if (TO_STR == "string" || TO_STR == "str") {
        return ScanDataType::STRING;
    }
    if (TO_STR == "bytearray" || TO_STR == "bytes") {
        return ScanDataType::BYTEARRAY;
    }
    return std::nullopt;
}

/**
 * @brief Parse match type from string with aliases support
 * @param tok Token to parse (e.g., "=", "eq", "changed")
 * @return Optional ScanMatchType
 */
[[nodiscard]] inline auto parseMatchType(std::string_view tok)
    -> std::optional<ScanMatchType> {
    const auto MATCH_STR = toLower(std::string(tok));
    if (MATCH_STR == "any") {
        return ScanMatchType::MATCHANY;
    }
    if (MATCH_STR == "eq" || MATCH_STR == "=") {
        return ScanMatchType::MATCHEQUALTO;
    }
    if (MATCH_STR == "neq" || MATCH_STR == "!=") {
        return ScanMatchType::MATCHNOTEQUALTO;
    }
    if (MATCH_STR == "gt" || MATCH_STR == ">") {
        return ScanMatchType::MATCHGREATERTHAN;
    }
    if (MATCH_STR == "lt" || MATCH_STR == "<") {
        return ScanMatchType::MATCHLESSTHAN;
    }
    if (MATCH_STR == "range") {
        return ScanMatchType::MATCHRANGE;
    }
    if (MATCH_STR == "changed") {
        return ScanMatchType::MATCHCHANGED;
    }
    if (MATCH_STR == "notchanged" || MATCH_STR == "update") {
        return ScanMatchType::MATCHNOTCHANGED;
    }
    if (MATCH_STR == "inc" || MATCH_STR == "increased") {
        return ScanMatchType::MATCHINCREASED;
    }
    if (MATCH_STR == "dec" || MATCH_STR == "decreased") {
        return ScanMatchType::MATCHDECREASED;
    }
    if (MATCH_STR == "incby") {
        return ScanMatchType::MATCHINCREASEDBY;
    }
    if (MATCH_STR == "decby") {
        return ScanMatchType::MATCHDECREASEDBY;
    }
    return std::nullopt;
}

/**
 * @brief Parse integer (supports hex with 0x prefix)
 * @param str String to parse
 * @return Optional int64_t value
 */
[[nodiscard]] inline auto parseInt64(std::string_view str)
    -> std::optional<int64_t> {
    if (str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        int64_t value{};
        const auto* begin = str.data() + 2;
        const auto* end = str.data() + str.size();
        auto [ptr, ec] = std::from_chars(begin, end, value, 16);
        if (ec == std::errc{}) {
            return value;
        }
        return std::nullopt;
    }
    int64_t value{};
    const auto* begin = str.data();
    const auto* end = str.data() + str.size();
    auto [ptr, ec] = std::from_chars(begin, end, value, 10);
    if (ec == std::errc{}) {
        return value;
    }
    return std::nullopt;
}

/**
 * @brief Parse floating point number
 * @param str String to parse
 * @return Optional double value
 */
[[nodiscard]] inline auto parseDouble(std::string_view str)
    -> std::optional<double> {
    try {
        return std::stod(std::string{str});
    } catch (...) {
        return std::nullopt;
    }
}

/**
 * @brief Build UserValue from parsed arguments
 * @param dataType Data type of the value
 * @param matchType Match type (affects whether range is needed)
 * @param args Argument list
 * @param startIndex Index to start parsing from
 * @return Optional UserValue
 */
[[nodiscard]] inline auto buildUserValue(ScanDataType dataType,
                                         ScanMatchType matchType,
                                         const std::vector<std::string>& args,
                                         size_t startIndex)
    -> std::optional<UserValue> {
    if (!matchNeedsUserValue(matchType)) {
        return UserValue{};  // empty flags indicates not used
    }

    auto needRange = (matchType == ScanMatchType::MATCHRANGE);
    if (needRange && (startIndex + 1 >= args.size())) {
        return std::nullopt;
    }

    switch (dataType) {
        case ScanDataType::INTEGER8:
        case ScanDataType::INTEGER16:
        case ScanDataType::INTEGER32:
        case ScanDataType::INTEGER64:
        case ScanDataType::ANYINTEGER: {
            auto lowOpt = parseInt64(args[startIndex]);
            if (!lowOpt) {
                return std::nullopt;
            }
            if (needRange) {
                auto highOpt = parseInt64(args[startIndex + 1]);
                if (!highOpt) {
                    return std::nullopt;
                }
                return UserValue::fromRange<int64_t>(*lowOpt, *highOpt);
            }
            return UserValue::fromScalar<int64_t>(*lowOpt);
        }
        case ScanDataType::FLOAT32:
        case ScanDataType::FLOAT64:
        case ScanDataType::ANYFLOAT:
        case ScanDataType::ANYNUMBER: {
            auto lowOpt = parseDouble(args[startIndex]);
            if (!lowOpt) {
                return std::nullopt;
            }
            if (needRange) {
                auto highOpt = parseDouble(args[startIndex + 1]);
                if (!highOpt) {
                    return std::nullopt;
                }
                return UserValue::fromRange<double>(*lowOpt, *highOpt);
            }
            return UserValue::fromScalar<double>(*lowOpt);
        }
        case ScanDataType::STRING: {
            if (startIndex >= args.size()) {
                return std::nullopt;
            }
            UserValue userVal;
            userVal.stringValue = args[startIndex];
            userVal.flags = MatchFlags::STRING;
            return userVal;
        }
        case ScanDataType::BYTEARRAY: {
            if (startIndex >= args.size()) {
                return std::nullopt;
            }
            std::string byteStr = args[startIndex];

            // 移除 0x 前缀
            if (byteStr.starts_with("0x") || byteStr.starts_with("0X")) {
                byteStr = byteStr.substr(2);
            }

            // 移除空格
            std::erase(byteStr, ' ');

            // 每两个字符是一个字节
            if (byteStr.size() % 2 != 0) {
                return std::nullopt;
            }

            std::vector<std::uint8_t> bytes;
            for (size_t i = 0; i < byteStr.size(); i += 2) {
                std::string byteHex = byteStr.substr(i, 2);
                std::uint8_t byte = 0;
                auto result = std::from_chars(
                    byteHex.data(), byteHex.data() + byteHex.size(), byte, 16);
                if (result.ec != std::errc{}) {
                    return std::nullopt;
                }
                bytes.push_back(byte);
            }

            UserValue userVal;
            userVal.bytearrayValue = bytes;
            userVal.flags = MatchFlags::BYTEARRAY;
            return userVal;
        }
        default:
            return std::nullopt;
    }
}

}  // namespace value
