module;
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

export module value;

export import value.flags;
import utils.endianness;

export struct Value {
    MatchFlags flags = MatchFlags::EMPTY;
    std::vector<std::uint8_t> bytes;

    Value() = default;

    // Construct from raw data
    explicit Value(const std::uint8_t* data, std::size_t len)
        : bytes(data, data + len) {}

    explicit Value(std::vector<std::uint8_t> data) : bytes(std::move(data)) {}

    // set bytes
    void setBytes(const std::uint8_t* data, std::size_t len) {
        bytes.assign(data, data + len);
    }

    void setBytes(const std::vector<std::uint8_t>& data) { bytes = data; }

    void setBytes(std::vector<std::uint8_t>&& data) { bytes = std::move(data); }

    // Get byte view
    [[nodiscard]] auto data() const noexcept -> const std::uint8_t* {
        return bytes.data();
    }

    [[nodiscard]] auto data() noexcept -> std::uint8_t* { return bytes.data(); }

    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return bytes.size();
    }

    [[nodiscard]] auto empty() const noexcept -> bool { return bytes.empty(); }

    void clear() {
        bytes.clear();
        flags = MatchFlags::EMPTY;
    }
};

// ============================================================================
// Scalar byte packing/unpacking utilities
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
// UserValue: User input value (for scan matching)
// ============================================================================
export struct UserValue {
    Value raw;
    std::optional<Value> rangeHigh;  // For range scanning
    std::optional<std::vector<std::uint8_t>>
        byteMask;  // 0xFF=fixed, 0x00=wildcard (only for byte arrays)

    // Accessors
    [[nodiscard]] auto flags() const noexcept -> MatchFlags {
        return raw.flags;
    }
    [[nodiscard]] auto data() const noexcept -> const std::uint8_t* {
        return raw.data();
    }
    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return raw.size();
    }

    template <typename T>
    static auto fromScalar(T value,
                           utils::Endianness endianness = utils::getHost())
        -> UserValue {
        static_assert(std::is_trivially_copyable_v<T>);
        UserValue userValue;
        userValue.raw.flags = flagForType<T>();
        userValue.raw.bytes = packScalarBytes<T>(value, endianness);
        return userValue;
    }

    template <typename T>
    static auto fromScalarRange(T lowValue, T highValue,
                                utils::Endianness endianness = utils::getHost())
        -> UserValue {
        static_assert(std::is_trivially_copyable_v<T>);
        UserValue userValue;
        userValue.raw.flags = flagForType<T>();
        userValue.raw.bytes = packScalarBytes<T>(lowValue, endianness);
        userValue.rangeHigh = Value{};
        userValue.rangeHigh->flags = flagForType<T>();
        userValue.rangeHigh->bytes = packScalarBytes<T>(highValue, endianness);
        return userValue;
    }

    static auto fromString(std::string val) -> UserValue {
        UserValue userValue;
        userValue.raw.flags = MatchFlags::STRING;
        userValue.raw.bytes.assign(val.begin(), val.end());
        return userValue;
    }

    static auto fromByteArray(std::vector<std::uint8_t> val,
                              std::optional<std::vector<std::uint8_t>> mask =
                                  std::nullopt) -> UserValue {
        UserValue userValue;
        userValue.raw.flags = MatchFlags::BYTE_ARRAY;
        userValue.raw.bytes = std::move(val);
        userValue.byteMask = std::move(mask);
        return userValue;
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

    [[nodiscard]] auto byteArrayValue() const noexcept
        -> std::optional<std::span<const std::uint8_t>> {
        if (raw.flags != MatchFlags::BYTE_ARRAY) {
            return std::nullopt;
        }
        return std::span<const std::uint8_t>(raw.bytes.data(),
                                             raw.bytes.size());
    }

    [[nodiscard]] auto stringValue() const noexcept
        -> std::optional<std::string_view> {
        if (raw.flags != MatchFlags::STRING) {
            return std::nullopt;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return std::string_view{reinterpret_cast<const char*>(raw.data()),
                                raw.size()};
    }
};

// ============================================================================
// UserValue helpers
// ============================================================================
namespace value_detail {
template <typename T>
[[nodiscard]] inline auto unpackFromValue(const Value& source) noexcept
    -> std::optional<T> {
    static_assert(std::is_trivially_copyable_v<T>);
    const auto REQUIRED = flagForType<T>();
    if ((source.flags & REQUIRED) == MatchFlags::EMPTY) {
        return std::nullopt;
    }
    return unpackScalarBytes<T>(source.data(), source.size());
}
}  // namespace value_detail

export template <typename T>
[[nodiscard]] inline auto userValueAs(const UserValue& userValue) noexcept
    -> std::optional<T> {
    return value_detail::unpackFromValue<T>(userValue.raw);
}

export template <typename T>
[[nodiscard]] inline auto userValueHighAs(const UserValue& userValue) noexcept
    -> std::optional<T> {
    if (userValue.rangeHigh) {
        if (auto val = value_detail::unpackFromValue<T>(*userValue.rangeHigh)) {
            return val;
        }
    }
    return userValueAs<T>(userValue);
}

// ============================================================================
// std::formatter for UserValue
// ============================================================================
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
            if (auto text = userValue.stringValue()) {
                return format_to(ctx.out(), "\"{}\"", *text);
            }
            return format_to(ctx.out(), "\"\"");
        }
        format_to(ctx.out(), "[");
        for (std::size_t i = 0; i < userValue.size(); ++i) {
            if (i != 0U) {
                format_to(ctx.out(), " ");
            }
            format_to(ctx.out(), "{:02x}", userValue.data()[i]);
        }
        return format_to(ctx.out(), "]");
    }
};
}  // namespace std
