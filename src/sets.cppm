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
    size_t size;
    std::vector<size_t> buf;

    void clear() {
        size = 0;
        buf.clear();
    }

    // 静态成员函数，用于比较两个 size_t 值
    static int cmp(const size_t& i1, const size_t& i2) {
        auto result = i1 <=> i2;
        if (result == std::strong_ordering::less) {
            return -1;
        } else if (result == std::strong_ordering::greater) {
            return 1;
        } else {
            return 0;
        }
    }
};

[[deprecated(
    "This interface is deprecated. Please use std::vector's capacity "
    "management "
    "instead C-style array.")]]
constexpr auto inc_arr_sz =
    [](size_t** valarr, size_t* arr_maxsz, size_t maxsz) -> bool {
    if (*arr_maxsz > maxsz / 2) {
        *arr_maxsz = maxsz;
    } else {
        *arr_maxsz *= 2;
    }

    // 使用 std::realloc 进行内存重新分配
    void* valarr_tmpptr = std::realloc(*valarr, *arr_maxsz * sizeof(size_t));
    if (valarr_tmpptr == nullptr) {
        return false;  // 分配失败
    }

    *valarr = static_cast<size_t*>(valarr_tmpptr);  // 转换为正确的类型
    return true;
};

export bool parse_uintset(std::string_view lptr, Set& set, size_t maxSZ) {
    set.clear();
    std::vector<size_t> result;
    bool invert = false;
    std::string input(lptr);

    // 支持十进制和0x前缀十六进制
    auto num_rule = qi::uint_parser<size_t, 10, 1, 20>() |
                    (qi::lit("0x") >> qi::uint_parser<size_t, 16, 1, 16>());

    auto range_rule = (num_rule >> ".." >> num_rule)[phoenix::bind(
        [&](size_t a, size_t b) {
            if (a > b || b >= maxSZ) throw std::runtime_error("invalid range");
            for (size_t i = a; i <= b; ++i) result.push_back(i);
        },
        qi::_1, qi::_2)];

    auto single_rule = num_rule[phoenix::bind(
        [&](size_t v) {
            if (v >= maxSZ) throw std::runtime_error("number out of bounds");
            result.push_back(v);
        },
        qi::_1)];

    auto expr = -(qi::lit('!')[phoenix::ref(invert) = true]) >>
                ((range_rule | single_rule) % ',');

    try {
        auto first = input.begin(), last = input.end();
        bool ok = qi::parse(first, last, expr) && first == last;
        if (!ok) return false;
    } catch (const std::exception&) {
        return false;
    }

    if (result.empty()) return false;

    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    if (result.back() >= maxSZ) return false;

    if (invert) {
        if (result.size() == maxSZ) return false;
        std::vector<size_t> inv;
        size_t idx = 0;
        for (size_t i = 0; i < maxSZ; ++i) {
            if (idx < result.size() && result[idx] == i)
                ++idx;
            else
                inv.push_back(i);
        }
        result = std::move(inv);
    }

    set.buf = std::move(result);
    set.size = set.buf.size();
    return true;
}