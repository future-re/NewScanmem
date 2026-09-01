#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace utils {

class StringUtils {
   public:
    [[nodiscard]] static auto toLower(std::string_view str) -> std::string;
    [[nodiscard]] static auto toUpper(std::string_view str) -> std::string;
    [[nodiscard]] static auto trim(std::string_view str) -> std::string_view;
    [[nodiscard]] static auto trimLeft(std::string_view str)
        -> std::string_view;
    [[nodiscard]] static auto trimRight(std::string_view str)
        -> std::string_view;
    [[nodiscard]] static auto split(std::string_view str, char delimiter)
        -> std::vector<std::string_view>;
    [[nodiscard]] static auto split(std::string_view str,
                                    std::string_view delimiters)
        -> std::vector<std::string_view>;
    [[nodiscard]] static auto startsWith(std::string_view str,
                                         std::string_view prefix) -> bool;
    [[nodiscard]] static auto endsWith(std::string_view str,
                                       std::string_view suffix) -> bool;
    [[nodiscard]] static auto contains(std::string_view str,
                                       std::string_view substring) -> bool;
    [[nodiscard]] static auto join(const std::vector<std::string_view>& strings,
                                   std::string_view delimiter) -> std::string;
};

}  // namespace utils
