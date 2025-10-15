/**
 * @file parser.cppm
 * @brief Value parsing utilities for different data types
 */

module;

#include <unistd.h>

#include <cerrno>
#include <charconv>
#include <cstdint>
#include <expected>
#include <limits>
#include <string>
#include <string_view>
#include <system_error>

export module utils.parser;

export namespace utils {

/**
 * @class ValueParser
 * @brief Utility class for parsing string values to various types
 */
class ValueParser {
   public:
    /**
     * @brief Parse string to signed integer
     * @param str String to parse
     * @param base Number base (8, 10, 16, etc.)
     * @return Expected containing parsed value or error message
     */
    [[nodiscard]] static auto parseInteger(std::string_view str, int base = 10)
        -> std::expected<int64_t, std::string> {
        if (str.empty()) {
            return std::unexpected("Empty string");
        }

        int64_t value = 0;
        auto [ptr, ec] =
            std::from_chars(str.data(), str.data() + str.size(), value, base);

        if (ec == std::errc::invalid_argument) {
            return std::unexpected("Invalid integer format");
        }
        if (ec == std::errc::result_out_of_range) {
            return std::unexpected("Integer value out of range");
        }
        if (ptr != str.data() + str.size()) {
            return std::unexpected("Extra characters after integer");
        }

        return value;
    }

    /**
     * @brief Parse string to unsigned integer
     * @param str String to parse
     * @param base Number base (8, 10, 16, etc.)
     * @return Expected containing parsed value or error message
     */
    [[nodiscard]] static auto parseUInteger(std::string_view str, int base = 10)
        -> std::expected<uint64_t, std::string> {
        if (str.empty()) {
            return std::unexpected("Empty string");
        }

        uint64_t value = 0;
        auto [ptr, ec] =
            std::from_chars(str.data(), str.data() + str.size(), value, base);

        if (ec == std::errc::invalid_argument) {
            return std::unexpected("Invalid unsigned integer format");
        }
        if (ec == std::errc::result_out_of_range) {
            return std::unexpected("Unsigned integer value out of range");
        }
        if (ptr != str.data() + str.size()) {
            return std::unexpected("Extra characters after unsigned integer");
        }

        return value;
    }

    /**
     * @brief Parse string to floating point number
     * @param str String to parse
     * @return Expected containing parsed value or error message
     */
    [[nodiscard]] static auto parseFloat(std::string_view str)
        -> std::expected<double, std::string> {
        if (str.empty()) {
            return std::unexpected("Empty string");
        }

        double value = 0.0;
        auto [ptr, ec] =
            std::from_chars(str.data(), str.data() + str.size(), value);

        if (ec == std::errc::invalid_argument) {
            return std::unexpected("Invalid float format");
        }
        if (ec == std::errc::result_out_of_range) {
            return std::unexpected("Float value out of range");
        }
        if (ptr != str.data() + str.size()) {
            return std::unexpected("Extra characters after float");
        }

        return value;
    }

    /**
     * @brief Parse hexadecimal address string to pointer
     * @param str Address string (with or without 0x prefix)
     * @return Expected containing parsed address or error message
     */
    [[nodiscard]] static auto parseAddress(std::string_view str)
        -> std::expected<void*, std::string> {
        if (str.empty()) {
            return std::unexpected("Empty address string");
        }

        // Remove 0x/0X prefix if present
        if (str.starts_with("0x") || str.starts_with("0X")) {
            str.remove_prefix(2);
        }

        uintptr_t addr = 0;
        auto [ptr, ec] =
            std::from_chars(str.data(), str.data() + str.size(), addr, 16);

        if (ec == std::errc::invalid_argument) {
            return std::unexpected("Invalid address format");
        }
        if (ec == std::errc::result_out_of_range) {
            return std::unexpected("Address value out of range");
        }
        if (ptr != str.data() + str.size()) {
            return std::unexpected("Extra characters after address");
        }

        return reinterpret_cast<void*>(addr);
    }

    /**
     * @brief Parse boolean value
     * @param str String to parse (true/false, yes/no, 1/0)
     * @return Expected containing parsed boolean or error message
     */
    [[nodiscard]] static auto parseBoolean(std::string_view str)
        -> std::expected<bool, std::string> {
        if (str.empty()) {
            return std::unexpected("Empty boolean string");
        }

        // Convert to lowercase for case-insensitive comparison
        std::string lowerStr;
        lowerStr.reserve(str.size());
        for (char chr : str) {
            lowerStr += static_cast<char>(
                std::tolower(static_cast<unsigned char>(chr)));
        }

        if (lowerStr == "true" || lowerStr == "yes" || lowerStr == "1" ||
            lowerStr == "on") {
            return true;
        }
        if (lowerStr == "false" || lowerStr == "no" || lowerStr == "0" ||
            lowerStr == "off") {
            return false;
        }

        return std::unexpected("Invalid boolean value");
    }

    /**
     * @brief Parse string based on base prefix (0x for hex, 0 for octal)
     * @param str String to parse
     * @return Expected containing parsed value or error message
     */
    [[nodiscard]] static auto parseIntegerAuto(std::string_view str)
        -> std::expected<int64_t, std::string> {
        if (str.empty()) {
            return std::unexpected("Empty string");
        }

        int base = 10;
        if (str.starts_with("0x") || str.starts_with("0X")) {
            base = 16;
            str.remove_prefix(2);
        } else if (str.starts_with("0") && str.size() > 1) {
            base = 8;
            str.remove_prefix(1);
        }

        return parseInteger(str, base);
    }

    /**
     * @brief Parse PID (process ID) with validation
     * @param str String to parse
     * @return Expected containing parsed PID or error message
     */
    [[nodiscard]] static auto parsePid(std::string_view str)
        -> std::expected<pid_t, std::string> {
        auto result = parseInteger(str, 10);
        if (!result) {
            return std::unexpected(result.error());
        }

        if (*result <= 0) {
            return std::unexpected("PID must be positive");
        }

        if (*result > std::numeric_limits<pid_t>::max()) {
            return std::unexpected("PID value too large");
        }

        return static_cast<pid_t>(*result);
    }
};

}  // namespace utils
