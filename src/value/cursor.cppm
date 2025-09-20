
module;

#include <algorithm>
#include <bit>
#include <cstddef>
#include <optional>
#include <string_view>
#include <tuple>

export module value.cursor;

import value.view;

namespace detail {
template <typename T>
inline auto tryGetAt(const MemView& memView, size_t off, Endian endian)
    -> std::optional<T> {
    return memView.tryGetAt<T>(off, endian);
}
}  // namespace detail

// 模板化的顺序读取器
export template <typename ViewT>
struct CursorT {
    ViewT v;
    size_t pos{0};

    CursorT() = default;
    explicit CursorT(ViewT view) : v(view) {}

    [[nodiscard]] auto size() const noexcept -> size_t { return v.size(); }
    [[nodiscard]] auto position() const noexcept -> size_t { return pos; }
    [[nodiscard]] auto remaining() const noexcept -> size_t {
        return (pos <= v.size()) ? (v.size() - pos) : 0;
    }
    [[nodiscard]] auto eof() const noexcept -> bool { return pos >= v.size(); }

    [[nodiscard]] auto seek(size_t newPos) noexcept -> bool {
        if (newPos > v.size()) {
            return false;
        }
        pos = newPos;
        return true;
    }
    [[nodiscard]] auto skip(size_t n) noexcept -> bool {
        if (remaining() < n) {
            return false;
        }
        pos += n;
        return true;
    }
    [[nodiscard]] auto align(size_t kVal) noexcept -> bool {
        if (kVal == 0) {
            return true;
        }
        size_t mod = pos % kVal;
        if (mod == 0) {
            return true;
        }
        size_t pad = kVal - mod;
        return skip(pad);
    }

    [[nodiscard]] auto readBytes(size_t n) noexcept -> std::optional<ViewT> {
        if (remaining() < n) {
            return std::nullopt;
        }
        auto sub = v.subview(pos, n);
        if (!sub) {
            return std::nullopt;
        }
        pos += n;
        return sub;
    }

    // 读取定长字符串（零拷贝）
    [[nodiscard]] auto readString(size_t len) noexcept
        -> std::optional<std::string_view> {
        if (remaining() < len) {
            return std::nullopt;
        }
        const char* charP = std::bit_cast<const char*>(v.data() + pos);
        std::string_view stringView{charP, len};
        pos += len;
        return stringView;
    }

    // 读取以 \0 结尾的字符串（零拷贝）。maxLen 用于限制搜索范围。
    [[nodiscard]] auto readCString(
        size_t maxLen = static_cast<size_t>(-1)) noexcept
        -> std::optional<std::string_view> {
        size_t limit = std::min(maxLen, remaining());
        const auto* base = v.data() + pos;
        size_t index = 0;
        for (; index < limit; ++index) {
            if (base[index] == 0) {
                break;
            }
        }
        if (index == limit) {
            return std::nullopt;  // 未找到终止符
        }
        auto stringView =
            std::string_view{std::bit_cast<const char*>(base), index};
        pos += (index + 1);  // 跳过结尾 0
        return stringView;
    }

    template <typename T>
    [[nodiscard]] auto read(Endian eType = Endian::HOST) noexcept
        -> std::optional<T> {
        if (remaining() < sizeof(T)) {
            return std::nullopt;
        }
        auto val = detail::tryGetAt<T>(v, pos, eType);
        if (!val) {
            return std::nullopt;
        }
        pos += sizeof(T);
        return val;
    }

    template <typename T>
    [[nodiscard]] auto peek(Endian eType = Endian::HOST) const noexcept
        -> std::optional<T> {
        return detail::tryGetAt<T>(v, pos, eType);
    }
};

// 导出便捷别名
export using Cursor = CursorT<MemView>;

// 结构体解析骨架：按顺序读取一组字段，返回 tuple
namespace detail {
template <std::size_t I = 0, typename... Ts, typename ViewT>
inline auto readTuple(CursorT<ViewT>& cur, std::tuple<Ts...>& tupleInput,
                      Endian endian) -> bool {
    if constexpr (I == sizeof...(Ts)) {
        return true;
    } else {
        using T = std::tuple_element_t<I, std::tuple<Ts...>>;
        auto val = cur.template read<T>(endian);
        if (!val) {
            return false;
        }
        std::get<I>(tupleInput) = *val;
        return readTuple<I + 1, Ts...>(cur, tupleInput, endian);
    }
}
}  // namespace detail

export template <typename... Ts, typename ViewT>
auto readStruct(CursorT<ViewT>& cur, Endian eType = Endian::HOST)
    -> std::optional<std::tuple<Ts...>> {
    std::tuple<Ts...> tup;
    auto pos0 = cur.position();
    bool okVal = detail::readTuple(cur, tup, eType);
    if (!okVal) {
        [[maybe_unused]] bool seekResult = cur.seek(pos0);
        return std::nullopt;
    }
    return tup;
}
