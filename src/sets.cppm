module;

#include <algorithm>
#include <boost/regex.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/spirit/include/qi.hpp>
#include <cctype>
#include <compare>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

export module sets;

namespace qi = boost::spirit::qi;
namespace phoenix = boost::phoenix;

constexpr size_t DEFAULT_UINTLS_SZ = 64;

export struct Set {
    std::vector<size_t> buf;

    [[nodiscard]] auto size() const -> size_t { return buf.size(); }

    void clear() { buf.clear(); }

    // 静态成员函数，用于比较两个 size_t 值
    static auto cmp(const size_t& lval, const size_t& rval) -> int {
        auto result = lval <=> rval;
        if (result == std::strong_ordering::less) {
            return -1;
        }
        if (result == std::strong_ordering::greater) {
            return 1;
        }
        return 0;
    }
};

namespace {
void parseRange(size_t aVal, size_t bVal, size_t maxSZ,
                std::vector<size_t>& result) {
    if (aVal > bVal || bVal >= maxSZ) {
        throw std::runtime_error("invalid range");
    }
    for (size_t i = aVal; i <= bVal; ++i) {
        result.push_back(i);
    }
}

void parseSingle(size_t val, size_t maxSZ, std::vector<size_t>& result) {
    if (val >= maxSZ) {
        throw std::runtime_error("number out of bounds");
    }
    result.push_back(val);
}

auto handleInvert(std::vector<size_t>& result, size_t maxSZ, bool invert)
    -> bool {
    if (result.empty() && invert) {
        for (size_t i = 0; i < maxSZ; ++i) {
            result.push_back(i);
        }
        return false;
    }
    return invert;
}

void applyInvert(std::vector<size_t>& result, size_t maxSZ) {
    if (result.size() == maxSZ) {
        throw std::runtime_error("cannot invert full set");
    }
    std::vector<size_t> inv;
    size_t idx = 0;
    for (size_t i = 0; i < maxSZ; ++i) {
        if (idx < result.size() && result[idx] == i) {
            ++idx;
        } else {
            inv.push_back(i);
        }
    }
    result = std::move(inv);
}
}  // namespace

export auto parseUintSet(std::string_view lptr, Set& set, size_t maxSZ)
    -> bool {
    set.clear();
    std::vector<size_t> result;
    bool invert = false;
    std::string input(lptr);

    auto hexRule = qi::lit("0x") >> qi::uint_parser<size_t, 16, 1, 16>();
    auto decRule = qi::uint_parser<size_t, 10, 1, 20>();
    auto numRule = hexRule | decRule;

    auto rangeRule = (numRule >> ".." >> numRule)[phoenix::bind(
        parseRange, qi::_1, qi::_2, maxSZ, phoenix::ref(result))];
    auto singleRule = numRule[phoenix::bind(parseSingle, qi::_1, maxSZ,
                                            phoenix::ref(result))];

    auto expr = -(qi::lit('!')[phoenix::ref(invert) = true]) >>
                ((rangeRule | singleRule) % ',');

    try {
        auto first = input.begin();
        auto last = input.end();
        if (!qi::parse(first, last, expr) || first != last) {
            return false;
        }
    } catch (const std::exception&) {
        return false;
    }

    std::ranges::sort(result);
    result.erase(std::ranges::unique(result).begin(), result.end());

    invert = handleInvert(result, maxSZ, invert);

    if (!result.empty() && result.back() >= maxSZ) {
        return false;
    }

    if (invert) {
        applyInvert(result, maxSZ);
    }

    set.buf = std::move(result);
    return true;
}