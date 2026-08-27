#pragma once

/**
 * @file core.hpp
 * @brief Core single-value container
 */

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "newscanmem/value/flags.hpp"

struct Value;

// ============================================================================
// Value: core single-value container
// ============================================================================
struct Value {
    MatchFlags flags = MatchFlags::EMPTY;
    std::vector<std::uint8_t> bytes;

    // For byte array matching: 0xFF=fixed, 0x00=wildcard
    std::optional<std::vector<std::uint8_t>> mask;

   private:
    template <ValueArithmeticType T>
    [[nodiscard]] static auto makeArithmetic(T value) -> Value {
        using U = std::remove_cvref_t<T>;
        Value val;
        val.flags = flagForType<U>();
        auto raw = std::bit_cast<std::array<std::uint8_t, sizeof(U)>>(
            static_cast<U>(value));
        val.bytes.assign(raw.begin(), raw.end());
        return val;
    }

    [[nodiscard]] static auto makeString(std::string value) -> Value;

    [[nodiscard]] static auto makeBytes(
        std::vector<std::uint8_t> data,
        std::optional<std::vector<std::uint8_t>> mask = std::nullopt) -> Value;

   public:
    Value() = default;

    explicit Value(const std::uint8_t* data, std::size_t len);

    explicit Value(std::vector<std::uint8_t> data);

    // ====================================================================
    // Canonical factories for single values
    // ====================================================================
    template <ValueArithmeticType T>
    [[nodiscard]] static auto of(T value) -> Value {
        return makeArithmetic(value);
    }

    template <ValueArithmeticType T>
    [[nodiscard]] static auto fromScalar(T value) -> Value {
        return of(value);
    }

    template <ValueStringType T>
    [[nodiscard]] static auto of(T&& value) -> Value {
        return makeString(std::string(std::forward<T>(value)));
    }

    template <ValueStringType T>
    [[nodiscard]] static auto fromString(T&& value) -> Value {
        return of(std::forward<T>(value));
    }

    [[nodiscard]] static auto fromString(const char* value) -> Value;

    template <ValueByteVectorType T>
    [[nodiscard]] static auto of(
        T&& data,
        std::optional<std::vector<std::uint8_t>> mask = std::nullopt) -> Value {
        return makeBytes(std::vector<std::uint8_t>(std::forward<T>(data)),
                         std::move(mask));
    }

    template <ValueByteVectorType T>
    [[nodiscard]] static auto fromByteArray(
        T&& data,
        std::optional<std::vector<std::uint8_t>> mask = std::nullopt) -> Value {
        return of(std::forward<T>(data), std::move(mask));
    }

    // ====================================================================
    // Canonical accessors for single values
    // ====================================================================

    template <ValueArithmeticType T>
    [[nodiscard]] auto as() const -> std::optional<T> {
        if (bytes.size() != sizeof(T)) {
            return std::nullopt;
        }
        T result{};
        std::copy_n(bytes.data(), sizeof(T), (std::uint8_t*)(&result));
        return result;
    }

    [[nodiscard]] auto asString() const -> std::optional<std::string>;

    [[nodiscard]] auto asBytes() const
        -> std::optional<std::vector<std::uint8_t>>;

    // ====================================================================
    // Basic operations
    // ====================================================================

    [[nodiscard]] auto data() const noexcept -> const std::uint8_t* {
        return bytes.data();
    }
    [[nodiscard]] auto data() noexcept -> std::uint8_t* { return bytes.data(); }
    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return bytes.size();
    }
    [[nodiscard]] auto empty() const noexcept -> bool { return bytes.empty(); }
    [[nodiscard]] auto flag() const noexcept -> MatchFlags { return flags; }

    void clear();

    void setBytes(const std::uint8_t* data, std::size_t len);

    void setBytes(const std::vector<std::uint8_t>& data);
    void setBytes(std::vector<std::uint8_t>&& data);

    [[nodiscard]] auto hasMask() const -> bool { return mask.has_value(); }
};

// ============================================================================
// UserValue: scan-facing wrapper around one or two Values
// ============================================================================
struct UserValue {
    Value primary;
    std::optional<Value> secondary;

    UserValue() = default;

    explicit UserValue(Value value);

    UserValue(Value lowValue, Value highValue);

    template <ValueArithmeticType T>
    [[nodiscard]] static auto of(T value) -> UserValue {
        return UserValue{Value::of(value)};
    }

    template <ValueArithmeticType T>
    [[nodiscard]] static auto fromScalar(T value) -> UserValue {
        return of(value);
    }

    template <ValueStringType T>
    [[nodiscard]] static auto of(T&& value) -> UserValue {
        return UserValue{Value::of(std::forward<T>(value))};
    }

    template <ValueStringType T>
    [[nodiscard]] static auto fromString(T&& value) -> UserValue {
        return of(std::forward<T>(value));
    }

    [[nodiscard]] static auto fromString(const char* value) -> UserValue;

    template <ValueByteVectorType T>
    [[nodiscard]] static auto of(T&& data,
                                 std::optional<std::vector<std::uint8_t>> mask =
                                     std::nullopt) -> UserValue {
        return UserValue{Value::of(std::forward<T>(data), std::move(mask))};
    }

    template <ValueByteVectorType T>
    [[nodiscard]] static auto fromByteArray(
        T&& data, std::optional<std::vector<std::uint8_t>> mask = std::nullopt)
        -> UserValue {
        return of(std::forward<T>(data), std::move(mask));
    }

    [[nodiscard]] auto flag() const noexcept -> MatchFlags {
        return primary.flag();
    }

    [[nodiscard]] auto size() const noexcept -> std::size_t {
        return primary.size();
    }

    [[nodiscard]] auto empty() const noexcept -> bool {
        return primary.empty();
    }

    [[nodiscard]] auto data() const noexcept -> const std::uint8_t* {
        return primary.data();
    }

    [[nodiscard]] auto stringValue() const -> std::optional<std::string> {
        return primary.asString();
    }

    [[nodiscard]] auto byteArrayValue() const
        -> std::optional<std::vector<std::uint8_t>> {
        return primary.asBytes();
    }

    [[nodiscard]] auto hasMask() const -> bool { return primary.hasMask(); }

    [[nodiscard]] auto hasSecondary() const noexcept -> bool {
        return secondary.has_value();
    }
};

// ============================================================================
// std::formatter
// ============================================================================
namespace std {

template <>
struct formatter<Value> {
    static constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const Value& value, format_context& ctx) {
        if (value.flag() == MatchFlags::EMPTY) {
            return format_to(ctx.out(), "<empty>");
        }
        if (value.flag() == MatchFlags::STRING) {
            if (auto text = value.asString()) {
                return format_to(ctx.out(), "\"{}\"", *text);
            }
            return format_to(ctx.out(), "\"\"");
        }
        format_to(ctx.out(), "[");
        for (std::size_t i = 0; i < value.size(); ++i) {
            if (i != 0U) {
                format_to(ctx.out(), " ");
            }
            format_to(ctx.out(), "{:02x}", value.data()[i]);
        }
        return format_to(ctx.out(), "]");
    }
};

template <>
struct formatter<UserValue> {
    static constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const UserValue& value, format_context& ctx) {
        if (value.secondary) {
            return format_to(ctx.out(), "{}..{}", value.primary,
                             *value.secondary);
        }
        return format_to(ctx.out(), "{}", value.primary);
    }
};

}  // namespace std
