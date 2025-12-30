module;
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

export module value;

export import value.flags;
import utils.endianness;

export struct Value {
    MatchFlags flags = MatchFlags::EMPTY;
    std::vector<std::uint8_t> bytes;

    Value() = default;

    // 从原始数据构造
    explicit Value(const std::uint8_t* data, std::size_t len)
        : bytes(data, data + len) {}

    explicit Value(std::vector<std::uint8_t> data) : bytes(std::move(data)) {}

    // 设置字节内容
    void setBytes(const std::uint8_t* data, std::size_t len) {
        bytes.assign(data, data + len);
    }

    void setBytes(const std::vector<std::uint8_t>& data) { bytes = data; }

    void setBytes(std::vector<std::uint8_t>&& data) { bytes = std::move(data); }

    // 获取字节视图
    [[nodiscard]] auto data() const noexcept -> const std::uint8_t* {
        return bytes.data();
    }

    [[nodiscard]] auto data() noexcept -> std::uint8_t* { return bytes.data(); }

    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return bytes.size();
    }

    [[nodiscard]] auto empty() const noexcept -> bool { return bytes.empty(); }

    // 清空
    void clear() {
        bytes.clear();
        flags = MatchFlags::EMPTY;
    }
};

// ============================================================================
// 标量字节打包/解包工具
// ============================================================================
export template <typename T>
auto packScalarBytes(T val, utils::Endianness endianness = utils::getHost())
    -> std::vector<std::uint8_t> {
    static_assert(std::is_trivially_copyable_v<T>);
    std::vector<std::uint8_t> buf(sizeof(T));
    std::memcpy(buf.data(), &val, sizeof(T));
    if (endianness != utils::getHost()) {
        std::ranges::reverse(buf);
    }
    return buf;
}

export template <typename T>
[[nodiscard]] auto unpackScalarBytes(
    const std::uint8_t* data, std::size_t len,
    utils::Endianness endianness = utils::getHost()) -> std::optional<T> {
    static_assert(std::is_trivially_copyable_v<T>);
    if (len < sizeof(T)) {
        return std::nullopt;
    }
    T temp{};
    if (endianness != utils::getHost()) {
        std::array<std::uint8_t, sizeof(T)> revData{};
        std::reverse_copy(data, data + sizeof(T), revData.begin());
        std::memcpy(&temp, revData.data(), sizeof(T));
    } else {
        std::memcpy(&temp, data, sizeof(T));
    }
    return temp;
}

// ============================================================================
// UserValue: 用户输入的值（用于扫描匹配）
// ============================================================================
export struct UserValue {
    Value raw;
    std::optional<Value> rangeHigh;  // 用于范围扫描
    std::optional<std::vector<std::uint8_t>>
        byteMask;  // 0xFF=fixed, 0x00=wildcard（仅用于字节数组）

    // 访问器
    [[nodiscard]] auto flags() const noexcept -> MatchFlags {
        return raw.flags;
    }
    [[nodiscard]] auto data() const noexcept -> const std::uint8_t* {
        return raw.data();
    }
    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return raw.size();
    }

    // --- 标量构造器 ---
    template <typename T>
    static auto fromScalar(T value,
                           utils::Endianness endianness = utils::getHost())
        -> UserValue {
        static_assert(std::is_trivially_copyable_v<T>);
        UserValue uv;
        uv.raw.flags = flagForType<T>();
        uv.raw.bytes = packScalarBytes<T>(value, endianness);
        return uv;
    }

    template <typename T>
    static auto fromScalarRange(T lowValue, T highValue,
                                utils::Endianness endianness = utils::getHost())
        -> UserValue {
        static_assert(std::is_trivially_copyable_v<T>);
        UserValue uv;
        uv.raw.flags = flagForType<T>();
        uv.raw.bytes = packScalarBytes<T>(lowValue, endianness);
        uv.rangeHigh = Value{};
        uv.rangeHigh->flags = flagForType<T>();
        uv.rangeHigh->bytes = packScalarBytes<T>(highValue, endianness);
        return uv;
    }

    // --- 字符串构造器 ---
    static auto fromString(std::string val) -> UserValue {
        UserValue uv;
        uv.raw.flags = MatchFlags::STRING;
        uv.raw.bytes.assign(val.begin(), val.end());
        return uv;
    }

    // --- 字节数组构造器 ---
    static auto fromByteArray(std::vector<std::uint8_t> val,
                              std::optional<std::vector<std::uint8_t>> mask =
                                  std::nullopt) -> UserValue {
        UserValue uv;
        uv.raw.flags = MatchFlags::BYTE_ARRAY;
        uv.raw.bytes = std::move(val);
        uv.byteMask = std::move(mask);
        return uv;
    }

    [[nodiscard]] auto flag() const -> MatchFlags { return raw.flags; }

    void setFlag(MatchFlags newFlag) {
        raw.flags = newFlag;
        if (rangeHigh) {
            rangeHigh->flags = newFlag;
        }
    }

    [[nodiscard]] auto byteData() const -> const std::vector<std::uint8_t>& {
        return raw.bytes;
    }

    [[nodiscard]] auto byteDataHigh() const
        -> std::optional<std::vector<std::uint8_t>> {
        if (rangeHigh) {
            return rangeHigh->bytes;
        }
        return std::nullopt;
    }

    // --- 标量值提取 ---
    template <typename T>
    [[nodiscard]] auto value(utils::Endianness endianness =
                                 utils::getHost()) const -> std::optional<T> {
        return unpackScalarBytes<T>(raw.data(), raw.size(), endianness);
    }

    template <typename T>
    [[nodiscard]] auto valueHigh(
        utils::Endianness endianness = utils::getHost()) const
        -> std::optional<T> {
        if (rangeHigh) {
            return unpackScalarBytes<T>(rangeHigh->data(), rangeHigh->size(),
                                        endianness);
        }
        return std::nullopt;
    }

    [[nodiscard]] auto byteArrayValue() const -> std::vector<std::uint8_t> {
        if (raw.flags != MatchFlags::BYTE_ARRAY) {
            return {};
        }
        return raw.bytes;
    }

    [[nodiscard]] auto stringValue() const -> std::string {
        if (raw.flags != MatchFlags::STRING) {
            return {};
        }
        return {reinterpret_cast<const char*>(raw.data()), raw.size()};
    }
};

// ============================================================================
// UserValue 辅助函数
// ============================================================================
namespace value_detail {
template <typename T>
[[nodiscard]] inline auto unpackFromValue(const Value& source) noexcept
    -> std::optional<T> {
    static_assert(std::is_trivially_copyable_v<T>);
    const auto required = flagForType<T>();
    if ((source.flags & required) == MatchFlags::EMPTY) {
        return std::nullopt;
    }
    return unpackScalarBytes<T>(source.data(), source.size());
}
}  // namespace value_detail

export template <typename T>
[[nodiscard]] inline auto userValueAs(const UserValue& uv) noexcept
    -> std::optional<T> {
    return value_detail::unpackFromValue<T>(uv.raw);
}

export template <typename T>
[[nodiscard]] inline auto userValueHighAs(const UserValue& uv) noexcept
    -> std::optional<T> {
    if (uv.rangeHigh) {
        if (auto val = value_detail::unpackFromValue<T>(*uv.rangeHigh)) {
            return val;
        }
    }
    return userValueAs<T>(uv);
}

// ============================================================================
// std::formatter 特化
// ============================================================================
namespace std {
template <>
struct formatter<UserValue> {
    static constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const UserValue& uv, format_context& ctx) {
        if (uv.flags() == MatchFlags::EMPTY) {
            return format_to(ctx.out(), "<empty>");
        }
        if (uv.flags() == MatchFlags::STRING) {
            return format_to(ctx.out(), "\"{}\"", uv.stringValue());
        }
        format_to(ctx.out(), "[");
        for (std::size_t i = 0; i < uv.size(); ++i) {
            if (i != 0U) {
                format_to(ctx.out(), " ");
            }
            format_to(ctx.out(), "{:02x}", uv.data()[i]);
        }
        return format_to(ctx.out(), "]");
    }
};
}  // namespace std
