#include "memseek/utils/string.hpp"

#include <algorithm>
#include <cctype>
#include <ranges>

namespace utils {

auto StringUtils::toLower(std::string_view str) -> std::string {
    std::string result(str);
    std::ranges::transform(result, result.begin(), [](const unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return result;
}

auto StringUtils::toUpper(std::string_view str) -> std::string {
    std::string result(str);
    std::ranges::transform(result, result.begin(), [](const unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return result;
}

auto StringUtils::trim(std::string_view str) -> std::string_view {
    return trimLeft(trimRight(str));
}

auto StringUtils::trimLeft(std::string_view str) -> std::string_view {
    const auto it = std::ranges::find_if_not(
        str, [](const unsigned char c) { return std::isspace(c) != 0; });
    return str.substr(static_cast<std::size_t>(it - str.begin()));
}

auto StringUtils::trimRight(std::string_view str) -> std::string_view {
    const auto it = std::ranges::find_if_not(
        str | std::views::reverse,
        [](const unsigned char c) { return std::isspace(c) != 0; });
    return it == str.rend()
               ? std::string_view{}
               : str.substr(0,
                            static_cast<std::size_t>(it.base() - str.begin()));
}

auto StringUtils::split(std::string_view str,
                        const char delimiter) -> std::vector<std::string_view> {
    std::vector<std::string_view> result;
    std::size_t start = 0;
    while (start <= str.size()) {
        const auto end = str.find(delimiter, start);
        result.emplace_back(str.substr(start, end - start));
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    return result;
}

auto StringUtils::split(std::string_view str, const std::string_view delimiters)
    -> std::vector<std::string_view> {
    std::vector<std::string_view> result;
    std::size_t start = 0;
    while (start < str.size()) {
        start = str.find_first_not_of(delimiters, start);
        if (start == std::string_view::npos) break;
        const auto end = str.find_first_of(delimiters, start);
        result.emplace_back(str.substr(start, end - start));
        if (end == std::string_view::npos) break;
        start = end + 1;
    }
    return result;
}

auto StringUtils::startsWith(const std::string_view str,
                             const std::string_view prefix) -> bool {
    return str.starts_with(prefix);
}

auto StringUtils::endsWith(const std::string_view str,
                           const std::string_view suffix) -> bool {
    return str.ends_with(suffix);
}

auto StringUtils::contains(const std::string_view str,
                           const std::string_view substring) -> bool {
    return str.contains(substring);
}

auto StringUtils::join(const std::vector<std::string_view>& strings,
                       const std::string_view delimiter) -> std::string {
    if (strings.empty()) return {};

    std::string result;
    std::size_t size = (strings.size() - 1) * delimiter.size();
    for (const auto string : strings) size += string.size();
    result.reserve(size);

    for (std::size_t index = 0; index < strings.size(); ++index) {
        if (index != 0) result.append(delimiter);
        result.append(strings[index]);
    }
    return result;
}

}  // namespace utils
