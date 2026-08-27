#pragma once

#include <compare>
#include <cstddef>
#include <string_view>
#include <vector>

constexpr std::size_t DEFAULT_UINTS_SZ = 64;

struct Set {
    std::vector<std::size_t> buf;
    [[nodiscard]] auto size() const -> std::size_t { return buf.size(); }
    void clear() { buf.clear(); }
    static auto cmp(const std::size_t& left, const std::size_t& right) -> int {
        const auto result = left <=> right;
        return result == std::strong_ordering::less      ? -1
               : result == std::strong_ordering::greater ? 1
                                                         : 0;
    }
};

[[nodiscard]] auto parseUintSet(std::string_view text, Set& set,
                                std::size_t max_size) -> bool;
