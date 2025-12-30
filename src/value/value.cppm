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

// Value: 底层字节容器，只负责存储原始字节数据
// 不包含任何标量类型解析逻辑，那些由 value.scalar 模块处理
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

    [[nodiscard]] auto data() noexcept -> std::uint8_t* {
        return bytes.data();
    }

    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return bytes.size();
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return bytes.empty();
    }

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
[[nodiscard]] auto unpackScalarBytes(const std::uint8_t* data, std::size_t len,
                                     utils::Endianness endianness = utils::getHost())
    -> std::optional<T> {
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
// ScalarKind: 精确的标量类型标识
// ============================================================================
export enum class ScalarKind : std::uint8_t {
    U8,
    S8,
    U16,
    S16,
    U32,
    S32,
    U64,
    S64,
    F32,
    F64
};

export using ScalarVariant =
    std::variant<std::uint8_t, std::int8_t, std::uint16_t, std::int16_t,
                 std::uint32_t, std::int32_t, std::uint64_t, std::int64_t, float,
                 double>;

// ============================================================================
// ScalarKind 辅助函数
// ============================================================================
export constexpr auto sizeOf(ScalarKind kType) noexcept -> std::size_t {
    switch (kType) {
        case ScalarKind::U8:
        case ScalarKind::S8:
            return 1;
        case ScalarKind::U16:
        case ScalarKind::S16:
            return 2;
        case ScalarKind::U32:
        case ScalarKind::S32:
        case ScalarKind::F32:
            return 4;
        case ScalarKind::U64:
        case ScalarKind::S64:
        case ScalarKind::F64:
            return 8;
    }
    return 0;
}

export constexpr auto isSigned(ScalarKind kType) noexcept -> bool {
    switch (kType) {
        case ScalarKind::S8:
        case ScalarKind::S16:
        case ScalarKind::S32:
        case ScalarKind::S64:
            return true;
        default:
            return false;
    }
}

export constexpr auto isFloatingPoint(ScalarKind kType) noexcept -> bool {
    return kType == ScalarKind::F32 || kType == ScalarKind::F64;
}

// ScalarKind -> MatchFlags 转换
export constexpr auto toMatchFlags(ScalarKind kType) noexcept -> MatchFlags {
    switch (kType) {
        case ScalarKind::U8:
        case ScalarKind::S8:
            return MatchFlags::B8;
        case ScalarKind::U16:
        case ScalarKind::S16:
            return MatchFlags::B16;
        case ScalarKind::U32:
        case ScalarKind::S32:
        case ScalarKind::F32:
            return MatchFlags::B32;
        case ScalarKind::U64:
        case ScalarKind::S64:
        case ScalarKind::F64:
            return MatchFlags::B64;
    }
    return MatchFlags::EMPTY;
}

// ============================================================================
// KindOf<T>: 类型到 ScalarKind 的编译期映射
// ============================================================================
export template <typename T>
struct KindOf;

export template <>
struct KindOf<std::uint8_t> {
    static constexpr ScalarKind VALUE = ScalarKind::U8;
};

export template <>
struct KindOf<std::int8_t> {
    static constexpr ScalarKind VALUE = ScalarKind::S8;
};

export template <>
struct KindOf<std::uint16_t> {
    static constexpr ScalarKind VALUE = ScalarKind::U16;
};

export template <>
struct KindOf<std::int16_t> {
    static constexpr ScalarKind VALUE = ScalarKind::S16;
};

export template <>
struct KindOf<std::uint32_t> {
    static constexpr ScalarKind VALUE = ScalarKind::U32;
};

export template <>
struct KindOf<std::int32_t> {
    static constexpr ScalarKind VALUE = ScalarKind::S32;
};

export template <>
struct KindOf<std::uint64_t> {
    static constexpr ScalarKind VALUE = ScalarKind::U64;
};

export template <>
struct KindOf<std::int64_t> {
    static constexpr ScalarKind VALUE = ScalarKind::S64;
};

export template <>
struct KindOf<float> {
    static constexpr ScalarKind VALUE = ScalarKind::F32;
};

export template <>
struct KindOf<double> {
    static constexpr ScalarKind VALUE = ScalarKind::F64;
};

// 便捷函数
export template <typename T>
constexpr auto kindOf() noexcept -> ScalarKind {
    return KindOf<T>::VALUE;
}

// ============================================================================
// ScalarValue: 带类型标记的标量值
// ============================================================================
export struct ScalarValue {
    ScalarKind kind{};
    ScalarVariant value;

    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t {
        return sizeOf(kind);
    }

    // 从具体类型构造
    template <typename T>
    static auto make(const T& val) noexcept -> ScalarValue {
        static_assert(std::is_trivially_copyable_v<T>);
        static_assert(sizeof(T) <= 8, "ScalarValue supports up to 8 bytes");
        ScalarValue out{KindOf<T>::VALUE};
        out.value = val;
        return out;
    }

    template <typename T>
    [[nodiscard]] auto get() const noexcept -> std::optional<T> {
        static_assert(std::is_trivially_copyable_v<T>);
        if (const auto* pval = std::get_if<T>(&value)) {
            return *pval;
        }
        return std::nullopt;
    }

    static auto fromBytes(ScalarKind kVal, const std::uint8_t* data,
                          std::size_t len) noexcept -> std::optional<ScalarValue> {
        if (len < sizeOf(kVal)) {
            return std::nullopt;
        }

        auto impl = [kVal, data]<typename T>() {
            T val{};
            std::memcpy(&val, data, sizeof(T));
            ScalarValue out{.kind = kVal};
            out.value = val;
            return out;
        };

        switch (kVal) {
            case ScalarKind::U8:
                return impl.template operator()<std::uint8_t>();
            case ScalarKind::S8:
                return impl.template operator()<std::int8_t>();
            case ScalarKind::U16:
                return impl.template operator()<std::uint16_t>();
            case ScalarKind::S16:
                return impl.template operator()<std::int16_t>();
            case ScalarKind::U32:
                return impl.template operator()<std::uint32_t>();
            case ScalarKind::S32:
                return impl.template operator()<std::int32_t>();
            case ScalarKind::U64:
                return impl.template operator()<std::uint64_t>();
            case ScalarKind::S64:
                return impl.template operator()<std::int64_t>();
            case ScalarKind::F32:
                return impl.template operator()<float>();
            case ScalarKind::F64:
                return impl.template operator()<double>();
        }
        return std::nullopt;
    }

    template <typename T>
    static auto readFromAddress(const void* address) noexcept
        -> std::optional<ScalarValue> {
        static_assert(std::is_trivially_copyable_v<T>,
                      "T must be trivially copyable");
        if (address == nullptr) {
            return std::nullopt;
        }
        T val;
        std::memcpy(&val, address, sizeof(T));
        return make<T>(val);
    }

    [[nodiscard]] auto asVariant() const noexcept -> ScalarVariant { return value; }

    [[nodiscard]] auto toMatchFlags() const noexcept -> MatchFlags {
        return ::toMatchFlags(kind);
    }
};

// ============================================================================
// UserValue: 用户输入的值（用于扫描匹配）
// ============================================================================
export struct UserValue {
    Value raw;
    std::optional<Value> rangeHigh;  // 用于范围扫描
    std::optional<std::vector<std::uint8_t>>
        byteMask;  // 0xFF=fixed, 0x00=wildcard（仅用于字节数组）

    // 访问器
    [[nodiscard]] auto flags() const noexcept -> MatchFlags { return raw.flags; }
    [[nodiscard]] auto data() const noexcept -> const std::uint8_t* {
        return raw.data();
    }
    [[nodiscard]] auto size() const noexcept -> std::size_t { return raw.size(); }

    // --- 标量构造器 ---
    template <typename T>
    static auto fromScalar(T value,
                           utils::Endianness endianness = utils::getHost()) -> UserValue {
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
                              std::optional<std::vector<std::uint8_t>> mask = std::nullopt)
        -> UserValue {
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

    [[nodiscard]] auto byteDataHigh() const -> std::optional<std::vector<std::uint8_t>> {
        if (rangeHigh) {
            return rangeHigh->bytes;
        }
        return std::nullopt;
    }

    // --- 标量值提取 ---
    template <typename T>
    [[nodiscard]] auto value(utils::Endianness endianness = utils::getHost()) const
        -> std::optional<T> {
        return unpackScalarBytes<T>(raw.data(), raw.size(), endianness);
    }

    template <typename T>
    [[nodiscard]] auto valueHigh(utils::Endianness endianness = utils::getHost()) const
        -> std::optional<T> {
        if (rangeHigh) {
            return unpackScalarBytes<T>(rangeHigh->data(), rangeHigh->size(), endianness);
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
    static constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

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
