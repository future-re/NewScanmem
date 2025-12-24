module;
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>  // For std::memcpy
#include <format>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

export module value;

export import value.scalar;
export import value.flags;
import utils.endianness;
export struct Value {
    MatchFlags flags = MatchFlags::EMPTY;
    std::vector<std::uint8_t> bytes;

    Value() = default;

    // Set byte content
    void setBytes(const std::uint8_t* data, std::size_t len) {
        bytes.assign(data, data + len);
    }

    void setBytes(const std::vector<std::uint8_t>& data) { bytes = data; }

    // Get byte view
    [[nodiscard]] auto data() const noexcept -> const std::uint8_t* {
        return bytes.data();
    }

    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return bytes.size();
    }
};

// Helper function - must be declared before UserValue
export template <typename T>
constexpr auto flagForScalarType() -> MatchFlags {
    if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
        return MatchFlags::B8;
    } else if constexpr (std::is_same_v<T, int16_t> ||
                         std::is_same_v<T, uint16_t>) {
        return MatchFlags::B16;
    } else if constexpr (std::is_same_v<T, int32_t> ||
                         std::is_same_v<T, uint32_t> ||
                         std::is_same_v<T, float>) {
        return MatchFlags::B32;
    } else if constexpr (std::is_same_v<T, int64_t> ||
                         std::is_same_v<T, uint64_t> ||
                         std::is_same_v<T, double>) {
        return MatchFlags::B64;
    } else if constexpr (std::is_same_v<T, std::string>) {
        return MatchFlags::STRING;
    } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
        return MatchFlags::BYTE_ARRAY;
    }
    return MatchFlags::EMPTY;
}

export template <typename T>
auto packScalarBytes(T val, utils::Endianness endianness = utils::getHost())
    -> std::vector<uint8_t> {
    std::vector<uint8_t> buf(sizeof(T));
    std::memcpy(buf.data(), &val, sizeof(T));
    if (endianness != utils::getHost()) {
        std::ranges::reverse(buf);
    }
    return buf;
}

template <typename T>
[[nodiscard]]
auto toScalarVal(Value val, utils::Endianness endianness = utils::getHost())
    -> T {
    T temp{};
    if (endianness != utils::getHost()) {
        std::vector<std::uint8_t> revData(val.size());
        std::reverse_copy(val.data(), val.data() + val.size(), revData.data());
        std::memcpy(&temp, revData.data(), sizeof(T));
        return temp;
    }
    std::memcpy(&temp, val.data(), sizeof(T));
    return temp;
}

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

    // 构造器
    template <typename T>
    static auto fromScalar(T value,
                           utils::Endianness endianness = utils::getHost())
        -> UserValue {
        UserValue userValue;
        userValue.raw.flags = flagForScalarType<T>();
        userValue.raw.bytes = packScalarBytes<T>(value, endianness);
        return userValue;
    }

    template <typename T>
    static auto fromScalarRange(T lowValue, T highValue,
                                utils::Endianness endianness = utils::getHost())
        -> UserValue {
        UserValue userValue;
        userValue.raw.flags = flagForScalarType<T>();
        userValue.raw.bytes = packScalarBytes<T>(lowValue, endianness);
        userValue.rangeHigh = Value{};
        userValue.rangeHigh->flags = flagForScalarType<T>();
        userValue.rangeHigh->bytes = packScalarBytes<T>(highValue, endianness);
        return userValue;
    }

    static auto fromString(std::string val) -> UserValue {
        UserValue userValue;
        userValue.raw.flags = MatchFlags::STRING;
        userValue.raw.bytes.assign(val.begin(), val.end());
        return userValue;
    }

    static auto fromByteArray(
        std::vector<uint8_t> val,
        std::optional<std::vector<uint8_t>> mask = std::nullopt) -> UserValue {
        UserValue userValue;
        userValue.raw.flags = MatchFlags::BYTE_ARRAY;
        userValue.raw.bytes = std::move(val);
        userValue.byteMask = std::move(mask);
        return userValue;
    }

    [[nodiscard]] auto flag() const -> MatchFlags { return raw.flags; }

    // 设置标志位的接口
    void setFlag(MatchFlags newFlag) {
        raw.flags = newFlag;
        if( rangeHigh) {
            rangeHigh->flags = newFlag;
        }
    }

    [[nodiscard]] auto byteData() const -> const std::vector<uint8_t>& {
        return raw.bytes;
    }
    [[nodiscard]] auto byteDataHigh() const
        -> std::optional<std::vector<uint8_t>> {
        if (rangeHigh) {
            return rangeHigh->bytes;
        }
        return std::nullopt;
    }

    template <typename T>
    [[nodiscard]] auto value() -> T {
        return toScalarVal<T>(raw);
    }

    template <typename T>
    [[nodiscard]] auto valueHigh() -> std::optional<T> {
        if (rangeHigh) {
            return toScalarVal<T>(*rangeHigh);
        }
        return std::nullopt;
    }

    [[nodiscard]] auto byteArrayValue() const -> std::vector<uint8_t> {
        if (raw.flags != MatchFlags::BYTE_ARRAY) {
            return std::vector<uint8_t>{};
        }
        return raw.bytes;
    }

    [[nodiscard]] auto stringValue() const -> std::string {
        if (raw.flags != MatchFlags::STRING) {
            return std::string{};
        }
        return std::string(
            static_cast<const char*>(static_cast<const void*>(raw.data())),
            raw.size());
    }
};

namespace value_detail {
template <typename T>
[[nodiscard]] inline auto unpackScalar(const Value& source) noexcept
    -> std::optional<T> {
    static_assert(std::is_trivially_copyable_v<T>,
                  "userValue conversions require trivially copyable types");
    const auto REQUIRED = flagForScalarType<T>();
    if ((source.flags & REQUIRED) == MatchFlags::EMPTY) {
        return std::nullopt;
    }
    if (source.size() < sizeof(T)) {
        return std::nullopt;
    }

    std::array<std::uint8_t, sizeof(T)> buffer{};
    std::memcpy(buffer.data(), source.data(), sizeof(T));
    if (utils::getHost() == utils::Endianness::BIG) {
        std::ranges::reverse(buffer);
    }

    T out{};
    std::memcpy(&out, buffer.data(), sizeof(T));
    return out;
}
}  // namespace value_detail

export template <typename T>
[[nodiscard]] inline auto userValueAs(const UserValue& userValue) noexcept
    -> std::optional<T> {
    return value_detail::unpackScalar<T>(userValue.raw);
}

export template <typename T>
[[nodiscard]] inline auto userValueHighAs(const UserValue& userValue) noexcept
    -> std::optional<T> {
    if (userValue.rangeHigh) {
        if (auto scalarOpt =
                value_detail::unpackScalar<T>(*userValue.rangeHigh)) {
            return scalarOpt;
        }
    }
    return userValueAs<T>(userValue);
}

namespace std {
template <>
struct formatter<UserValue> {
    static constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const UserValue& userValue, format_context& ctx) {
        if (userValue.flags() == MatchFlags::EMPTY) {
            return format_to(ctx.out(), "<empty>");
        }
        if (userValue.flags() == MatchFlags::STRING) {
            std::string str(static_cast<const char*>(
                                static_cast<const void*>(userValue.data())),
                            userValue.size());
            return format_to(ctx.out(), "\"{}\"", str);
        }
        format_to(ctx.out(), "[");
        for (size_t index = 0; index < userValue.size(); ++index) {
            if (index != 0U) {
                format_to(ctx.out(), " ");
            }
            format_to(ctx.out(), "{:02x}", userValue.data()[index]);
        }
        return format_to(ctx.out(), "]");
    }
};
}  // namespace std