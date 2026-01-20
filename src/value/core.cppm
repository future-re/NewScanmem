module;
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <optional>
#include <string>
#include <vector>

export module value.core;

import value.flags;
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
// UserValue: User input value (for scan matching)
// ============================================================================
export struct UserValue {
    Value byteValue;  // Underlying byte value
    std::optional<std::vector<std::uint8_t>>
        byteMask;  // 0xFF=fixed, 0x00=wildcard (only for byte arrays)

    // Accessors
    [[nodiscard]] auto flags() const noexcept -> MatchFlags {
        return byteValue.flags;
    }
    [[nodiscard]] auto data() const noexcept -> const std::uint8_t* {
        return byteValue.data();
    }
    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return byteValue.size();
    }

    template <NumericType T>
    static auto fromValue(T value) -> UserValue {
        UserValue userValue;
        userValue.byteValue.flags = flagForType<T>();
        auto bytes = std::bit_cast<std::array<std::uint8_t, sizeof(T)>>(value);
        userValue.byteValue.bytes.assign(bytes.begin(), bytes.end());
        return userValue;
    }

    template <StringType T>
    static auto fromValue(std::string val) -> UserValue {
        UserValue userValue;
        userValue.byteValue.flags = MatchFlags::STRING;
        userValue.byteValue.bytes.assign(val.begin(), val.end());
        return userValue;
    }

    template <ByteArrayType T>
    static auto fromValue(std::vector<std::uint8_t> val,
                          std::optional<std::vector<std::uint8_t>> mask =
                              std::nullopt) -> UserValue {
        UserValue userValue;
        userValue.byteValue.flags = MatchFlags::BYTE_ARRAY;
        userValue.byteValue.bytes = std::move(val);
        userValue.byteMask = std::move(mask);
        return userValue;
    }

    template <NumericType T>
    [[nodiscard]]
    auto getValue() const -> std::optional<T> {
        if (byteValue.size() != sizeof(T)) {
            return std::nullopt;
        }
        T resultValue{};
        std::copy_n(byteValue.data(), sizeof(T),
                    reinterpret_cast<std::uint8_t*>(&resultValue));
        return resultValue;
    }

    template <StringType T>
    [[nodiscard]]
    auto getValue() const -> std::optional<std::string> {
        if (byteValue.flags != MatchFlags::STRING) {
            return std::nullopt;
        }
        return std::string(byteValue.bytes.begin(), byteValue.bytes.end());
    }

    template <ByteArrayType T>
    [[nodiscard]]
    auto getValue() const -> std::optional<std::vector<std::uint8_t>> {
        if (byteValue.flags != MatchFlags::BYTE_ARRAY) {
            return std::nullopt;
        }
        return byteValue.bytes;
    }

    [[nodiscard]] auto flag() const -> MatchFlags { return byteValue.flags; }

    void setFlag(MatchFlags newFlag) { byteValue.flags = newFlag; }

    [[nodiscard]] auto byteData() const -> const std::vector<std::uint8_t>& {
        return byteValue.bytes;
    }
};

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
            if (auto text = userValue.getValue<std::string>()) {
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

export using UserValueRange = std::pair<UserValue, UserValue>;