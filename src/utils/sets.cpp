#include "newscanmem/utils/sets.hpp"

#include <algorithm>
#include <charconv>
#include <stdexcept>

namespace {
void parseRange(const std::size_t first, const std::size_t last,
                const std::size_t maximum, std::vector<std::size_t>& result) {
    if (first > last || last >= maximum)
        throw std::runtime_error("invalid range");
    for (std::size_t value = first; value <= last; ++value)
        result.push_back(value);
}
auto parseNumber(std::string_view token) -> std::size_t {
    auto base = 10;
    if (token.starts_with("0x") || token.starts_with("0X")) {
        token.remove_prefix(2);
        base = 16;
    }
    if (token.empty()) throw std::runtime_error("empty number");
    std::size_t value = 0;
    const auto [end, error] =
        std::from_chars(token.data(), token.data() + token.size(), value, base);
    if (error != std::errc{} || end != token.data() + token.size())
        throw std::runtime_error("invalid number");
    return value;
}
}  // namespace

auto parseUintSet(std::string_view text, Set& set,
                  const std::size_t maximum) -> bool {
    set.clear();
    if (text.empty()) return false;
    const auto invert = text.front() == '!';
    if (invert) text.remove_prefix(1);
    if (text.empty()) return false;
    std::vector<std::size_t> result;
    try {
        for (std::size_t cursor = 0; cursor < text.size();) {
            const auto comma = text.find(',', cursor);
            const auto token = text.substr(
                cursor, comma == std::string_view::npos ? text.size() - cursor
                                                        : comma - cursor);
            if (token.empty()) return false;
            const auto range = token.find("..");
            if (range == std::string_view::npos) {
                const auto value = parseNumber(token);
                if (value >= maximum)
                    throw std::runtime_error("number out of bounds");
                result.push_back(value);
            } else {
                if (token.find("..", range + 2) != std::string_view::npos)
                    return false;
                parseRange(parseNumber(token.substr(0, range)),
                           parseNumber(token.substr(range + 2)), maximum,
                           result);
            }
            if (comma == std::string_view::npos) break;
            cursor = comma + 1;
        }
    } catch (const std::exception&) {
        return false;
    }
    std::ranges::sort(result);
    result.erase(std::ranges::unique(result).begin(), result.end());
    if (invert) {
        if (result.empty()) {
            for (std::size_t value = 0; value < maximum; ++value)
                result.push_back(value);
        } else {
            if (result.size() == maximum) return false;
            std::vector<std::size_t> inverse;
            for (std::size_t value = 0, index = 0; value < maximum; ++value) {
                if (index < result.size() && result[index] == value)
                    ++index;
                else
                    inverse.push_back(value);
            }
            result = std::move(inverse);
        }
    }
    set.buf = std::move(result);
    return true;
}
