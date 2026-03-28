/**
 * @file types.cppm
 * @brief Core value types using std::variant for type safety
 *
 * This module replaces the old Value/UserValue design with a modern,
 * type-safe approach using std::variant. Eliminates reinterpret_cast
 * and provides clear type semantics.
 */

module;

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

export module value.types;

import value.flags;
import utils.endianness;

export namespace value {

// ============================================================================
// ByteArray with optional mask (for pattern matching)
// ============================================================================

struct ByteArray {
    std::vector<std::uint8_t> data;
    std::optional<std::vector<std::uint8_t>> mask;

    ByteArray() = default;
    explicit ByteArray(std::vector<std::uint8_t> d) : data(std::move(d)) {}
    ByteArray(std::vector<std::uint8_t> d, std::vector<std::uint8_t> m)
        : data(std::move(d)), mask(std::move(m)) {
        if (mask && mask->size() != data.size()) {
            mask = std::nullopt;  // Invalid mask, ignore it
        }
    }

    [[nodiscard]] auto empty() const noexcept -> bool { return data.empty(); }
    [[nodiscard]] auto size() const noexcept -> std::size_t { return data.size(); }
};

// ============================================================================
// Value variant - type-safe container for all value types
// ============================================================================

using Value = std::variant<
    std::monostate,                    // 0: empty
    int8_t,                            // 1: int8
    int16_t,                           // 2: int16
    int32_t,                           // 3: int32
    int64_t,                           // 4: int64
    uint8_t,                           // 5: uint8
    uint16_t,                          // 6: uint16
    uint32_t,                          // 7: uint32
    uint64_t,                          // 8: uint64
    float,                             // 9: float32
    double,                            // 10: float64
    std::string,                       // 11: string
    ByteArray                          // 12: byte array
>;

// ============================================================================
// Type predicates
// ============================================================================

[[nodiscard]] constexpr auto isEmpty(const Value& v) -> bool {
    return std::holds_alternative<std::monostate>(v);
}

[[nodiscard]] constexpr auto isString(const Value& v) -> bool {
    return std::holds_alternative<std::string>(v);
}

[[nodiscard]] constexpr auto isByteArray(const Value& v) -> bool {
    return std::holds_alternative<ByteArray>(v);
}

template<typename T>
[[nodiscard]] constexpr auto isType(const Value& v) -> bool {
    return std::holds_alternative<T>(v);
}

// ============================================================================
// Type-safe value getters
// ============================================================================

template<typename T>
[[nodiscard]] auto getAs(const Value& v) -> std::optional<T> {
    if (auto* ptr = std::get_if<T>(&v)) {
        return *ptr;
    }
    return std::nullopt;
}

// ============================================================================
// MatchFlags mapping
// ============================================================================

[[nodiscard]] constexpr auto getValueFlag(const Value& v) -> MatchFlags {
    return std::visit(
        [](auto&& arg) -> MatchFlags {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return MatchFlags::EMPTY;
            } else if constexpr (std::is_same_v<T, std::string>) {
                return MatchFlags::STRING;
            } else if constexpr (std::is_same_v<T, ByteArray>) {
                return MatchFlags::BYTE_ARRAY;
            } else {
                return flagForType<T>();
            }
        },
        v);
}

// ============================================================================
// Serialization utilities (type-safe, no reinterpret_cast)
// ============================================================================

template<typename T>
    requires(std::is_arithmetic_v<T>)
[[nodiscard]] auto scalarToBytes(T value, utils::Endianness endian) -> std::vector<std::uint8_t> {
    auto bytes = std::bit_cast<std::array<std::uint8_t, sizeof(T)>>(value);
    if (endian != utils::getHost()) {
        std::ranges::reverse(bytes);
    }
    return {bytes.begin(), bytes.end()};
}

template<typename T>
    requires(std::is_arithmetic_v<T>)
[[nodiscard]] auto bytesToScalar(std::span<const std::uint8_t> bytes, utils::Endianness endian) -> std::optional<T> {
    if (bytes.size() != sizeof(T)) {
        return std::nullopt;
    }
    std::array<std::uint8_t, sizeof(T)> arr{};
    std::copy_n(bytes.begin(), sizeof(T), arr.begin());
    if (endian != utils::getHost()) {
        std::ranges::reverse(arr);
    }
    return std::bit_cast<T>(arr);
}

[[nodiscard]] inline auto toBytes(const Value& v, utils::Endianness endian = utils::getHost())
    -> std::vector<std::uint8_t> {
    return std::visit(
        [&](auto&& arg) -> std::vector<std::uint8_t> {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return {};
            } else if constexpr (std::is_arithmetic_v<T>) {
                return scalarToBytes(arg, endian);
            } else if constexpr (std::is_same_v<T, std::string>) {
                return {arg.begin(), arg.end()};
            } else if constexpr (std::is_same_v<T, ByteArray>) {
                return arg.data;
            } else {
                return {};
            }
        },
        v);
}

[[nodiscard]] inline auto getByteSize(const Value& v) -> std::size_t {
    return std::visit(
        [](auto&& arg) -> std::size_t {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return 0;
            } else if constexpr (std::is_arithmetic_v<T>) {
                return sizeof(T);
            } else if constexpr (std::is_same_v<T, std::string>) {
                return arg.size();
            } else if constexpr (std::is_same_v<T, ByteArray>) {
                return arg.size();
            } else {
                return 0;
            }
        },
        v);
}

// ============================================================================
// Range value (for range matching)
// ============================================================================

struct ValueRange {
    Value low;
    Value high;

    [[nodiscard]] auto isValid() const -> bool {
        return !isEmpty(low) && !isEmpty(high) &&
               getValueFlag(low) == getValueFlag(high);
    }
};

// ============================================================================
// Legacy compatibility helpers
// ============================================================================

// Create Value from typed value
template<typename T>
[[nodiscard]] auto makeValue(T value) -> Value {
    return Value{value};
}

[[nodiscard]] inline auto makeString(std::string s) -> Value {
    return Value{std::move(s)};
}

[[nodiscard]] inline auto makeByteArray(std::vector<std::uint8_t> data,
                                        std::optional<std::vector<std::uint8_t>> mask = std::nullopt) -> Value {
    ByteArray ba;
    ba.data = std::move(data);
    ba.mask = std::move(mask);
    if (ba.mask && ba.mask->size() != ba.data.size()) {
        ba.mask = std::nullopt;
    }
    return Value{std::move(ba)};
}

}  // namespace value

// ============================================================================
// std::formatter specialization
// ============================================================================

namespace std {

template <>
struct formatter<value::Value> {
    static constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    static auto format(const value::Value& v, format_context& ctx) {
        return std::visit(
            [&](auto&& arg) -> format_context::iterator {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, std::monostate>) {
                    return format_to(ctx.out(), "<empty>");
                } else if constexpr (std::is_same_v<T, std::string>) {
                    return format_to(ctx.out(), "\"{}\"", arg);
                } else if constexpr (std::is_same_v<T, value::ByteArray>) {
                    format_to(ctx.out(), "[");
                    for (size_t i = 0; i < arg.data.size(); ++i) {
                        if (i != 0) format_to(ctx.out(), " ");
                        format_to(ctx.out(), "{:02x}", arg.data[i]);
                    }
                    return format_to(ctx.out(), "]");
                } else if constexpr (std::is_floating_point_v<T>) {
                    return format_to(ctx.out(), "{}", arg);
                } else if constexpr (std::is_integral_v<T>) {
                    return format_to(ctx.out(), "{}", arg);
                } else {
                    return format_to(ctx.out(), "<unknown>");
                }
            },
            v);
    }
};

}  // namespace std
