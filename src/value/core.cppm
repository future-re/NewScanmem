/**
 * @file core.cppm
 * @brief Simplified unified Value type with aggregated factory
 */

module;

#include <algorithm>
#include <array>
#include <bit>
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

export struct Value;

// ============================================================================
// Unified Value: Single type for all value operations
// ============================================================================
export struct Value {
    MatchFlags flags = MatchFlags::EMPTY;
    std::vector<std::uint8_t> bytes;

    // For range matching: upper bound value
    std::optional<std::vector<std::uint8_t>> secondaryBytes;

    // For byte array matching: 0xFF=fixed, 0x00=wildcard
    std::optional<std::vector<std::uint8_t>> mask;

    Value() = default;

    explicit Value(const std::uint8_t* data, std::size_t len)
        : bytes(data, data + len) {}

    explicit Value(std::vector<std::uint8_t> data) : bytes(std::move(data)) {}

    // ====================================================================
    // Unified factory: Single method to create any Value
    // ====================================================================

    /**
     * @brief Create a Value from any supported type
     *
     * Supported types:
     * - Numeric: int8_t, int16_t, int32_t, int64_t, uint8_t, uint16_t,
     * uint32_t, uint64_t, float, double
     * - std::string
     * - std::vector<std::uint8_t> (with optional mask)
     * - std::pair<T, T> for ranges
     */
    template <typename T>
    [[nodiscard]] static auto of(T value) -> Value {
        if constexpr (std::is_arithmetic_v<T>) {
            return fromScalar(value);
        } else {
            static_assert(std::is_arithmetic_v<T>,
                          "Unsupported type for Value::of()");
            return Value{};
        }
    }

    [[nodiscard]] static auto of(std::string value) -> Value {
        Value val;
        val.flags = MatchFlags::STRING;
        val.bytes.assign(value.begin(), value.end());
        return val;
    }

    [[nodiscard]] static auto of(
        std::vector<std::uint8_t> data,
        std::optional<std::vector<std::uint8_t>> mask = std::nullopt) -> Value {
        Value val;
        val.flags = MatchFlags::BYTE_ARRAY;
        val.bytes = std::move(data);
        val.mask = std::move(mask);
        if (val.mask && val.mask->size() != val.bytes.size()) {
            val.mask = std::nullopt;
        }
        return val;
    }

    // Range factory: Value::of(std::pair{10, 100})
    template <NumericType T>
    [[nodiscard]] static auto of(std::pair<T, T> range) -> Value {
        Value val = fromScalar(range.first);
        auto highBytes =
            std::bit_cast<std::array<std::uint8_t, sizeof(T)>>(range.second);
        val.secondaryBytes =
            std::vector<std::uint8_t>(highBytes.begin(), highBytes.end());
        return val;
    }

    // Legacy factory methods (kept for compatibility)
    template <NumericType T>
    [[nodiscard]] static auto fromScalar(T value) -> Value {
        Value val;
        val.flags = flagForType<T>();
        auto bytes = std::bit_cast<std::array<std::uint8_t, sizeof(T)>>(value);
        val.bytes.assign(bytes.begin(), bytes.end());
        return val;
    }

    [[nodiscard]] static auto fromString(std::string value) -> Value {
        return of(std::move(value));
    }

    [[nodiscard]] static auto fromByteArray(
        std::vector<std::uint8_t> data,
        std::optional<std::vector<std::uint8_t>> mask = std::nullopt) -> Value {
        return of(std::move(data), std::move(mask));
    }

    template <NumericType T>
    [[nodiscard]] static auto fromRange(T low, T high) -> Value {
        return of(std::pair{low, high});
    }

    // ====================================================================
    // Unified accessor: Get value as any type
    // ====================================================================

    template <NumericType T>
    [[nodiscard]] auto as() const -> std::optional<T> {
        if (bytes.size() != sizeof(T)) {
            return std::nullopt;
        }
        T result{};
        std::copy_n(bytes.data(), sizeof(T), (std::uint8_t*)(&result));
        return result;
    }

    [[nodiscard]] auto asString() const -> std::optional<std::string> {
        if (flags != MatchFlags::STRING) {
            return std::nullopt;
        }
        return std::string(bytes.begin(), bytes.end());
    }

    [[nodiscard]] auto asBytes() const
        -> std::optional<std::vector<std::uint8_t>> {
        if (flags != MatchFlags::BYTE_ARRAY) {
            return std::nullopt;
        }
        return bytes;
    }

    // Range accessor: get upper bound
    template <NumericType T>
    [[nodiscard]] auto asHigh() const -> std::optional<T> {
        if (!secondaryBytes || secondaryBytes->size() != sizeof(T)) {
            return std::nullopt;
        }
        T result{};
        std::copy_n(secondaryBytes->data(), sizeof(T),
                    (std::uint8_t*)(&result));
        return result;
    }

    // Legacy accessors (kept for compatibility)
    template <NumericType T>
    [[nodiscard]] auto getScalar() const -> std::optional<T> {
        return as<T>();
    }
    [[nodiscard]] auto getString() const -> std::optional<std::string> {
        return asString();
    }
    [[nodiscard]] auto getByteArray() const
        -> std::optional<std::vector<std::uint8_t>> {
        return asBytes();
    }

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

    void clear() {
        bytes.clear();
        secondaryBytes.reset();
        mask.reset();
        flags = MatchFlags::EMPTY;
    }

    void setBytes(const std::uint8_t* data, std::size_t len) {
        bytes.assign(data, data + len);
    }

    void setBytes(const std::vector<std::uint8_t>& data) { bytes = data; }
    void setBytes(std::vector<std::uint8_t>&& data) { bytes = std::move(data); }

    // ====================================================================
    // Utility
    // ====================================================================

    [[nodiscard]] auto isRange() const -> bool {
        return secondaryBytes.has_value();
    }

    [[nodiscard]] auto hasMask() const -> bool { return mask.has_value(); }
};

// ============================================================================
// Backward compatibility
// ============================================================================
export using UserValue = Value;
export using UserValueRange = std::pair<Value, Value>;

// Legacy functions
export template <NumericType T>
[[nodiscard]] inline auto userValueAs(const Value& value) -> std::optional<T> {
    return value.as<T>();
}

export template <NumericType T>
[[nodiscard]] inline auto userValueHighAs(const Value& value)
    -> std::optional<T> {
    return value.asHigh<T>();
}

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
        if (value.flags == MatchFlags::EMPTY) {
            return format_to(ctx.out(), "<empty>");
        }
        if (value.flags == MatchFlags::STRING) {
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

}  // namespace std
