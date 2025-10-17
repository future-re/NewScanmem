/**
 * @file string.cppm
 * @brief String utility functions for common string operations
 */

module;

#include <algorithm>
#include <cctype>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

export module utils.string;

export namespace utils {

/**
 * @class StringUtils
 * @brief Utility class for string manipulation operations
 */
class StringUtils {
   public:
    /**
     * @brief Convert string to lowercase
     * @param str Input string view
     * @return Lowercase version of the string
     */
    [[nodiscard]] static auto toLower(std::string_view str) -> std::string {
        std::string result;
        result.reserve(str.size());
        std::ranges::transform(
            str, std::back_inserter(result),
            [](unsigned char chr) { return std::tolower(chr); });
        return result;
    }

    /**
     * @brief Convert string to uppercase
     * @param str Input string view
     * @return Uppercase version of the string
     */
    [[nodiscard]] static auto toUpper(std::string_view str) -> std::string {
        std::string result;
        result.reserve(str.size());
        std::ranges::transform(
            str, std::back_inserter(result),
            [](unsigned char chr) { return std::toupper(chr); });
        return result;
    }

    /**
     * @brief Trim whitespace from both ends of string
     * @param str Input string view
     * @return Trimmed string view
     */
    [[nodiscard]] static auto trim(std::string_view str) -> std::string_view {
        // Trim from start
        const auto* start = std::ranges::find_if(
            str, [](unsigned char chr) { return !std::isspace(chr); });

        if (start == str.end()) {
            return {};
        }

        // Trim from end
        const auto* end = std::ranges::find_if(str | std::views::reverse,
                                               [](unsigned char chr) {
                                                   return !std::isspace(chr);
                                               })
                              .base();

        const auto START_IDX = std::distance(str.begin(), start);
        const auto LENGTH = std::distance(start, end);
        return str.substr(START_IDX, LENGTH);
    }

    /**
     * @brief Trim whitespace from start of string
     * @param str Input string view
     * @return Left-trimmed string view
     */
    [[nodiscard]] static auto trimLeft(std::string_view str)
        -> std::string_view {
        const auto* start = std::ranges::find_if(
            str, [](unsigned char chr) { return !std::isspace(chr); });

        if (start == str.end()) {
            return {};
        }

        const auto START_IDX = std::distance(str.begin(), start);
        return str.substr(START_IDX);
    }

    /**
     * @brief Trim whitespace from end of string
     * @param str Input string view
     * @return Right-trimmed string view
     */
    [[nodiscard]] static auto trimRight(std::string_view str)
        -> std::string_view {
        const auto* end = std::ranges::find_if(str | std::views::reverse,
                                               [](unsigned char chr) {
                                                   return !std::isspace(chr);
                                               })
                              .base();

        const auto LENGTH = std::distance(str.begin(), end);
        return str.substr(0, LENGTH);
    }

    /**
     * @brief Split string by single delimiter character
     * @param str Input string view
     * @param delim Delimiter character
     * @return Vector of split strings
     */
    [[nodiscard]] static auto split(std::string_view str, char delim)
        -> std::vector<std::string> {
        std::vector<std::string> result;
        size_t start = 0;
        size_t end = str.find(delim);

        while (end != std::string_view::npos) {
            result.emplace_back(str.substr(start, end - start));
            start = end + 1;
            end = str.find(delim, start);
        }

        result.emplace_back(str.substr(start));
        return result;
    }

    /**
     * @brief Split string by any character in delimiter set
     * @param str Input string view
     * @param delims Delimiter characters
     * @return Vector of split strings
     */
    [[nodiscard]] static auto split(std::string_view str,
                                    std::string_view delims)
        -> std::vector<std::string> {
        std::vector<std::string> result;
        size_t start = 0;
        size_t end = str.find_first_of(delims);

        while (end != std::string_view::npos) {
            if (end > start) {
                result.emplace_back(str.substr(start, end - start));
            }
            start = end + 1;
            end = str.find_first_of(delims, start);
        }

        if (start < str.size()) {
            result.emplace_back(str.substr(start));
        }

        return result;
    }

    /**
     * @brief Check if string starts with prefix
     * @param str Input string view
     * @param prefix Prefix to check
     * @return True if string starts with prefix
     */
    [[nodiscard]] static auto startsWith(std::string_view str,
                                         std::string_view prefix) -> bool {
        return str.size() >= prefix.size() && str.starts_with(prefix);
    }

    /**
     * @brief Check if string ends with suffix
     * @param str Input string view
     * @param suffix Suffix to check
     * @return True if string ends with suffix
     */
    [[nodiscard]] static auto endsWith(std::string_view str,
                                       std::string_view suffix) -> bool {
        return str.size() >= suffix.size() &&
               str.substr(str.size() - suffix.size()) == suffix;
    }

    /**
     * @brief Check if string contains substring
     * @param str Input string view
     * @param substr Substring to find
     * @return True if string contains substring
     */
    [[nodiscard]] static auto contains(std::string_view str,
                                       std::string_view substr) -> bool {
        return str.find(substr) != std::string_view::npos;
    }

    /**
     * @brief Join strings with delimiter
     * @param strings Vector of strings to join
     * @param delim Delimiter string
     * @return Joined string
     */
    [[nodiscard]] static auto join(const std::vector<std::string>& strings,
                                   std::string_view delim) -> std::string {
        if (strings.empty()) {
            return {};
        }

        std::string result = strings[0];
        for (size_t idx = 1; idx < strings.size(); ++idx) {
            result += delim;
            result += strings[idx];
        }

        return result;
    }
};

}  // namespace utils
