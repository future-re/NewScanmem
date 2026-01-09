/**
 * @file parser.cppm
 * @brief Value parsing utilities for scan operations
 */
module;
#include <unistd.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

export module value.parser;

import scan.types;
import value;

export namespace value {

/**
 * @brief Convert string to lowercase
 */
inline auto toLower(std::string str) -> std::string {
    std::ranges::transform(str, str.begin(), [](unsigned char charVal) {
        return static_cast<char>(std::tolower(charVal));
    });
    return str;
}

[[nodiscard]] inline auto isFloatToken(std::string_view str) -> bool {
    if (str.empty()) {
        return false;
    }
    for (char ch : str) {
        if (ch == '.' || ch == 'e' || ch == 'E') {
            return true;
        }
    }
    const auto lowered = toLower(std::string(str));
    return lowered == "nan" || lowered == "+nan" || lowered == "-nan" ||
           lowered == "inf" || lowered == "+inf" || lowered == "-inf" ||
           lowered == "infinity" || lowered == "+infinity" ||
           lowered == "-infinity";
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
        return ScanDataType::ANY_NUMBER;
    }
    if (TO_STR == "anyint" || TO_STR == "anyinteger") {
        return ScanDataType::ANY_INTEGER;
    }
    if (TO_STR == "anyfloat") {
        return ScanDataType::ANY_FLOAT;
    }
    if (TO_STR == "int") {
        return ScanDataType::INTEGER_32;
    }
    if (TO_STR == "int8" || TO_STR == "i8") {
        return ScanDataType::INTEGER_8;
    }
    if (TO_STR == "int16" || TO_STR == "i16") {
        return ScanDataType::INTEGER_16;
    }
    if (TO_STR == "int32" || TO_STR == "i32") {
        return ScanDataType::INTEGER_32;
    }
    if (TO_STR == "int64" || TO_STR == "i64") {
        return ScanDataType::INTEGER_64;
    }
    if (TO_STR == "float" || TO_STR == "f32" || TO_STR == "float_32" ||
        TO_STR == "float32") {
        return ScanDataType::FLOAT_32;
    }
    if (TO_STR == "double" || TO_STR == "f64" || TO_STR == "float_64" ||
        TO_STR == "float64") {
        return ScanDataType::FLOAT_64;
    }
    if (TO_STR == "string" || TO_STR == "str") {
        return ScanDataType::STRING;
    }
    if (TO_STR == "bytearray" || TO_STR == "bytes") {
        return ScanDataType::BYTE_ARRAY;
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
        return ScanMatchType::MATCH_ANY;
    }
    if (MATCH_STR == "eq" || MATCH_STR == "=") {
        return ScanMatchType::MATCH_EQUAL_TO;
    }
    if (MATCH_STR == "neq" || MATCH_STR == "!=") {
        return ScanMatchType::MATCH_NOT_EQUAL_TO;
    }
    if (MATCH_STR == "gt" || MATCH_STR == ">") {
        return ScanMatchType::MATCH_GREATER_THAN;
    }
    if (MATCH_STR == "lt" || MATCH_STR == "<") {
        return ScanMatchType::MATCH_LESS_THAN;
    }
    if (MATCH_STR == "range") {
        return ScanMatchType::MATCH_RANGE;
    }
    if (MATCH_STR == "changed") {
        return ScanMatchType::MATCH_CHANGED;
    }
    if (MATCH_STR == "notchanged" || MATCH_STR == "update") {
        return ScanMatchType::MATCH_NOT_CHANGED;
    }
    if (MATCH_STR == "inc" || MATCH_STR == "increased") {
        return ScanMatchType::MATCH_INCREASED;
    }
    if (MATCH_STR == "dec" || MATCH_STR == "decreased") {
        return ScanMatchType::MATCH_DECREASED;
    }
    if (MATCH_STR == "incby") {
        return ScanMatchType::MATCH_INCREASED_BY;
    }
    if (MATCH_STR == "decby") {
        return ScanMatchType::MATCH_DECREASED_BY;
    }
    if (MATCH_STR == "regex" || MATCH_STR == "re") {
        return ScanMatchType::MATCH_REGEX;
    }
    return std::nullopt;
}

/**
 * @brief Parse integer with optional hex prefix and sign
 * @tparam T Target integer type (int8_t, int16_t, int32_t, int64_t, etc.)
 * @param str String to parse
 * @return Optional value of type T with automatic range checking
 */
template <typename T>
    requires std::is_integral_v<T>
[[nodiscard]] inline auto parseInteger(std::string_view str)
    -> std::optional<T> {
    if (str.empty()) {
        return std::nullopt;
    }

    // 处理可选的符号
    size_t offset = 0;
    bool negative = false;
    if (str[0] == '-') {
        negative = true;
        offset = 1;
    } else if (str[0] == '+') {
        offset = 1;
    }

    auto body = std::string_view{str.data() + offset, str.size() - offset};

    // 检查 0x/0X 前缀
    int base = 10;
    if (body.size() > 2 && body[0] == '0' &&
        (body[1] == 'x' || body[1] == 'X')) {
        body = std::string_view{body.data() + 2, body.size() - 2};
        base = 16;
    }

    if (body.empty()) {
        return std::nullopt;
    }

    // 对于有符号类型，先解析为无符号以正确处理边界情况（如 int8 的 -128）
    if constexpr (std::is_signed_v<T>) {
        using UT = std::make_unsigned_t<T>;
        UT absValue = 0;
        const auto* end = body.data() + body.size();
        auto [ptr, ec] =
            std::from_chars(body.data(), end, absValue, base);

        if (ec != std::errc{} || ptr != end) {
            return std::nullopt;
        }

        if (negative) {
            // 检查负数范围：-128 对 int8_t 是 abs(128)
            constexpr UT MAX_NEG =
                static_cast<UT>(std::numeric_limits<T>::max()) + 1U;
            if (absValue > MAX_NEG) {
                return std::nullopt;
            }
            if (absValue == MAX_NEG) {
                return std::numeric_limits<T>::min();
            }
            return -static_cast<T>(absValue);
        }
        if (absValue > static_cast<UT>(std::numeric_limits<T>::max())) {
            return std::nullopt;
        }
        return static_cast<T>(absValue);
    } else {
        if (negative) {
            return std::nullopt;
        }
        // 无符号类型直接解析
        T value = 0;
        const auto* end = body.data() + body.size();
        auto [ptr, ec] = std::from_chars(body.data(), end, value, base);

        if (ec == std::errc{} && ptr == end) {
            return value;  // 无符号不接受负号
        }
        return std::nullopt;
    }
}

[[nodiscard]] inline auto parseDouble(std::string_view str)
    -> std::optional<double> {
    if (str.empty()) {
        return std::nullopt;
    }
    double value = 0.0;
    const auto* end = str.data() + str.size();
    auto [ptr, ec] = std::from_chars(str.data(), end, value);
    if (ec != std::errc{} || ptr != end) {
        return std::nullopt;
    }
    return value;
}

/**
 * @brief Parse hexadecimal address string to an integer address value
 * @param str Address string (with or without 0x prefix)
 * @return Optional containing parsed address value
 */
[[nodiscard]] inline auto parseAddress(std::string_view str)
    -> std::optional<std::uintptr_t> {
    if (str.empty()) {
        return std::nullopt;
    }

    // Remove 0x/0X prefix if present
    if (str.starts_with("0x") || str.starts_with("0X")) {
        str.remove_prefix(2);
    }

    if (str.empty()) {
        return std::nullopt;
    }

    std::uintptr_t addr = 0;
    auto [ptr, ec] =
        std::from_chars(str.data(), str.data() + str.size(), addr, 16);

    if (ec != std::errc{} || ptr != str.data() + str.size()) {
        return std::nullopt;
    }

    return addr;
}

/**
 * @brief Parse PID (process ID) with validation
 * @param str String to parse
 * @return Optional containing parsed PID
 */
[[nodiscard]] inline auto parsePid(std::string_view str)
    -> std::optional<pid_t> {
    auto result = parseInteger<int64_t>(str);
    if (!result) {
        return std::nullopt;
    }

    if (*result <= 0 || *result > std::numeric_limits<pid_t>::max()) {
        return std::nullopt;
    }

    return static_cast<pid_t>(*result);
}

/**
 * @brief Parse boolean value
 * @param str String to parse (true/false, yes/no, 1/0, on/off)
 * @return Optional containing parsed boolean
 */
[[nodiscard]] inline auto parseBoolean(std::string_view str)
    -> std::optional<bool> {
    if (str.empty()) {
        return std::nullopt;
    }

    auto lowerStr = toLower(std::string(str));

    if (lowerStr == "true" || lowerStr == "yes" || lowerStr == "1" ||
        lowerStr == "on") {
        return true;
    }
    if (lowerStr == "false" || lowerStr == "no" || lowerStr == "0" ||
        lowerStr == "off") {
        return false;
    }

    return std::nullopt;
}

template <typename F>
constexpr auto relTol() -> F {
    if constexpr (std::is_same_v<F, float>) {
        return static_cast<F>(1E-5F);
    } else {
        return static_cast<F>(1E-12);
    }
}

template <typename F>
constexpr auto absTol() -> F {
    if constexpr (std::is_same_v<F, float>) {
        return static_cast<F>(1E-6F);
    } else {
        return static_cast<F>(1E-12);
    }
}

template <typename F>
[[nodiscard]] inline auto almostEqual(F firstValue, F secondValue) noexcept
    -> bool {
    using std::fabs;
    const F DIFFERENCE_VALUE = fabs(firstValue - secondValue);
    const F SCALE_VALUE =
        std::max(F(1), std::max(fabs(firstValue), fabs(secondValue)));
    return DIFFERENCE_VALUE <= std::max(absTol<F>(), relTol<F>() * SCALE_VALUE);
}

/**
 * @brief Helper: build UserValue for scalar/range of type T
 * @tparam T Scalar type (int8_t, int16_t, int32_t, int64_t, float, double)
 * @tparam Parser Parser function type
 * @param needRange Whether to parse as range
 * @param args Argument list
 * @param startIndex Start index in args
 * @param parser Parser function (parseInteger<T> or parseDouble)
 * @return Optional UserValue
 */
template <typename T, typename Parser>
[[nodiscard]] inline auto buildScalarOrRange(
    bool needRange, const std::vector<std::string>& args, size_t startIndex,
    Parser parser) -> std::optional<UserValue> {
    auto lowOpt = parser(args[startIndex]);
    if (!lowOpt) {
        return std::nullopt;
    }

    if (needRange) {
        auto highOpt = parser(args[startIndex + 1]);
        if (!highOpt) {
            return std::nullopt;
        }
        return UserValue::fromScalarRange<T>(*lowOpt, *highOpt);
    }
    return UserValue::fromScalar<T>(*lowOpt);
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
        return std::nullopt;
    }

    auto needRange = (matchType == ScanMatchType::MATCH_RANGE);
    if (needRange && (startIndex + 1 >= args.size())) {
        return std::nullopt;
    }

    switch (dataType) {
        case ScanDataType::INTEGER_8:
            return buildScalarOrRange<int8_t>(needRange, args, startIndex,
                                              parseInteger<int8_t>);

        case ScanDataType::INTEGER_16:
            return buildScalarOrRange<int16_t>(needRange, args, startIndex,
                                               parseInteger<int16_t>);

        case ScanDataType::INTEGER_32:
            return buildScalarOrRange<int32_t>(needRange, args, startIndex,
                                               parseInteger<int32_t>);

        case ScanDataType::INTEGER_64:
        case ScanDataType::ANY_INTEGER:
            return buildScalarOrRange<int64_t>(needRange, args, startIndex,
                                               parseInteger<int64_t>);

        case ScanDataType::FLOAT_32:
        case ScanDataType::FLOAT_64:
        case ScanDataType::ANY_FLOAT:
            return buildScalarOrRange<double>(needRange, args, startIndex,
                                              parseDouble);

        case ScanDataType::ANY_NUMBER: {
            if (startIndex >= args.size()) {
                return std::nullopt;
            }
            bool floatInput = isFloatToken(args[startIndex]);
            if (needRange) {
                if (startIndex + 1 >= args.size()) {
                    return std::nullopt;
                }
                floatInput = floatInput || isFloatToken(args[startIndex + 1]);
            }
            if (floatInput) {
                return buildScalarOrRange<double>(needRange, args, startIndex,
                                                  parseDouble);
            }
            return buildScalarOrRange<int64_t>(needRange, args, startIndex,
                                               parseInteger<int64_t>);
        }

        case ScanDataType::STRING: {
            if (startIndex >= args.size()) {
                return std::nullopt;
            }
            auto first = UserValue::fromString(args[startIndex]);
            return std::make_optional(first);
        }

        case ScanDataType::BYTE_ARRAY: {
            if (startIndex >= args.size()) {
                return std::nullopt;
            }
            std::string byteStr = args[startIndex];

            // 移除 0x/0X 前缀
            if (byteStr.starts_with("0x") || byteStr.starts_with("0X")) {
                byteStr = byteStr.substr(2);
            }

            // 移除所有空格
            std::erase(byteStr, ' ');

            // 每两个字符是一个字节
            if (byteStr.size() % 2 != 0) {
                return std::nullopt;
            }

            std::vector<std::uint8_t> bytes;
            std::vector<std::uint8_t> mask;
            bytes.reserve(byteStr.size() / 2);
            mask.reserve(byteStr.size() / 2);

            auto parseNibble = [](char ch, std::uint8_t& value,
                                  std::uint8_t& maskValue) -> bool {
                if (ch == '?' || ch == '*') {
                    value = 0;
                    maskValue = 0;
                    return true;
                }
                if (ch >= '0' && ch <= '9') {
                    value = static_cast<std::uint8_t>(ch - '0');
                    maskValue = 0x0F;
                    return true;
                }
                if (ch >= 'a' && ch <= 'f') {
                    value = static_cast<std::uint8_t>(ch - 'a' + 10);
                    maskValue = 0x0F;
                    return true;
                }
                if (ch >= 'A' && ch <= 'F') {
                    value = static_cast<std::uint8_t>(ch - 'A' + 10);
                    maskValue = 0x0F;
                    return true;
                }
                return false;
            };

            // 使用 ranges 以步长 2 解析每个字节
            for (size_t i = 0; i < byteStr.size(); i += 2) {
                std::uint8_t hiVal = 0;
                std::uint8_t loVal = 0;
                std::uint8_t hiMask = 0;
                std::uint8_t loMask = 0;
                if (!parseNibble(byteStr[i], hiVal, hiMask) ||
                    !parseNibble(byteStr[i + 1], loVal, loMask)) {
                    return std::nullopt;
                }
                bytes.push_back(
                    static_cast<std::uint8_t>((hiVal << 4) | loVal));
                mask.push_back(
                    static_cast<std::uint8_t>((hiMask << 4) | loMask));
            }

            auto first =
                UserValue::fromByteArray(std::move(bytes), std::move(mask));
            return std::make_optional(first);
        }

        default:
            return std::nullopt;
    }
}

}  // namespace value
