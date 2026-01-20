/**
 * @file parserStr.cppm
 * @brief String parsing utilities for numeric and boolean values
 */

module;

#include <unistd.h>

#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

export module utils.parserStr;

import utils.string;

export namespace utils {

[[nodiscard]] inline auto isFloatToken(std::string_view str) -> bool {
    if (str.empty()) {
        return false;
    }
    for (char chStr : str) {
        if (chStr == '.' || chStr == 'e' || chStr == 'E') {
            return true;
        }
    }
    const auto LOWERED_STR = StringUtils::toLower(str);
    return LOWERED_STR == "nan" || LOWERED_STR == "+nan" ||
           LOWERED_STR == "-nan" || LOWERED_STR == "inf" ||
           LOWERED_STR == "+inf" || LOWERED_STR == "-inf" ||
           LOWERED_STR == "infinity" || LOWERED_STR == "+infinity" ||
           LOWERED_STR == "-infinity";
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

    // Handle optional symbols
    size_t offset = 0;
    bool negative = false;
    if (str[0] == '-') {
        negative = true;
        offset = 1;
    } else if (str[0] == '+') {
        offset = 1;
    }

    auto body = std::string_view{str.data() + offset, str.size() - offset};

    // Check for 0x/0X prefix
    int base = 10;
    if (body.size() > 2 && body[0] == '0' &&
        (body[1] == 'x' || body[1] == 'X')) {
        body = std::string_view{body.data() + 2, body.size() - 2};
        base = 16;
    }

    if (body.empty()) {
        return std::nullopt;
    }

    // For signed types, parse to unsigned first to properly handle edge cases
    // (like -128 for int8)
    if constexpr (std::is_signed_v<T>) {
        using UT = std::make_unsigned_t<T>;
        UT absValue = 0;
        const auto* end = body.data() + body.size();
        auto [ptr, ec] = std::from_chars(body.data(), end, absValue, base);

        if (ec != std::errc{} || ptr != end) {
            return std::nullopt;
        }

        if (negative) {
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
        T value = 0;
        const auto* end = body.data() + body.size();
        auto [ptr, ec] = std::from_chars(body.data(), end, value, base);

        if (ec == std::errc{} && ptr == end) {
            return value;
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

    auto lowerStr = StringUtils::toLower(str);

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

}  // namespace utils
