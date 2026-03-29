module;

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <span>
#include <type_traits>

export module utils.read_helpers;

import scan.types;
import value.flags;
import value.core;

// Helpers for byte reading, endianness conversion, and type traits
// Goal: provide unified reading and type-tagging utilities for
// numeric/bytes/string modules

// Conditional endianness swap (safe for integral/floating/other trivially
// copyable types)
template <typename T>
constexpr auto swapIfReverse(T value, bool reverse) noexcept -> T {
    if (!reverse) {
        return value;
    }
    if constexpr (std::is_integral_v<T>) {
        return std::byteswap(value);
    } else if constexpr (std::is_same_v<T, float>) {
        auto bits32 = std::bit_cast<uint32_t>(value);
        bits32 = std::byteswap(bits32);
        return std::bit_cast<float>(bits32);
    } else if constexpr (std::is_same_v<T, double>) {
        auto bits64 = std::bit_cast<uint64_t>(value);
        bits64 = std::byteswap(bits64);
        return std::bit_cast<double>(bits64);
    } else {
        std::cerr << "Warning: swapIfReverse received an unsupported type, no "
                     "endianness conversion performed!"
                  << std::endl;
        return value;
    }
}

// Safely read specified type and apply endianness conversion
export template <typename T>
[[nodiscard]] inline auto readTyped(const Value* memoryPtr, size_t memLength,
                                    bool reverseEndianness) noexcept
    -> std::optional<T> {
    if (memoryPtr == nullptr || memLength < sizeof(T) ||
        memoryPtr->size() < sizeof(T)) {
        return std::nullopt;
    }
    T val{};
    std::memcpy(&val, memoryPtr->data(), sizeof(T));
    return swapIfReverse<T>(val, reverseEndianness);
}

export template <typename T>
[[nodiscard]] inline auto readTyped(std::span<const std::uint8_t> bytes,
                                    bool reverseEndianness) noexcept
    -> std::optional<T> {
    if (bytes.size() < sizeof(T)) {
        return std::nullopt;
    }
    T val{};
    std::memcpy(&val, bytes.data(), sizeof(T));
    return swapIfReverse<T>(val, reverseEndianness);
}

export template <typename T>
[[nodiscard]] inline auto readTyped(const Value& value, bool reverseEndianness)
    noexcept -> std::optional<T> {
    return readTyped<T>(std::span<const std::uint8_t>(value.data(), value.size()),
                        reverseEndianness);
}

// Try to read old value from Value* (strictly checks flags and length)
export template <typename T>
[[nodiscard]] inline auto oldValueAs(const Value* valuePtr,
                                     bool reverseEndianness = false) noexcept
    -> std::optional<T> {
    if (!valuePtr) {
        return std::nullopt;
    }
    const auto REQUIRED = flagForType<T>();
    if ((valuePtr->flags & REQUIRED) == MatchFlags::EMPTY) {
        return std::nullopt;
    }
    if (valuePtr->size() < sizeof(T)) {
        return std::nullopt;
    }
    return readTyped<T>(*valuePtr, reverseEndianness);
}

export template <typename T>
[[nodiscard]] inline auto userValueAs(const UserValue& value,
                                      bool reverseEndianness = false) noexcept
    -> std::optional<T> {
    const auto REQUIRED = flagForType<T>();
    if ((value.flag() & REQUIRED) == MatchFlags::EMPTY) {
        return std::nullopt;
    }
    return readTyped<T>(value.primary, reverseEndianness);
}

export template <typename T>
[[nodiscard]] inline auto userValueHighAs(
    const UserValue& value, bool reverseEndianness = false) noexcept
    -> std::optional<T> {
    if (!value.secondary) {
        return std::nullopt;
    }
    const auto REQUIRED = flagForType<T>();
    if (((*value.secondary).flag() & REQUIRED) == MatchFlags::EMPTY) {
        return std::nullopt;
    }
    return readTyped<T>(*value.secondary, reverseEndianness);
}

export template <typename F>
constexpr auto relTol() -> F {
    if constexpr (std::is_same_v<F, float>) {
        return static_cast<F>(1E-5F);
    } else {
        return static_cast<F>(1E-12);
    }
}

export template <typename F>
constexpr auto absTol() -> F {
    if constexpr (std::is_same_v<F, float>) {
        return static_cast<F>(1E-6F);
    } else {
        return static_cast<F>(1E-12);
    }
}

export template <typename F>
[[nodiscard]] inline auto almostEqual(F firstValue,
                                      F secondValue) noexcept -> bool {
    using std::fabs;
    const F DIFFERENCE_VALUE = fabs(firstValue - secondValue);
    const F SCALE_VALUE =
        std::max(F(1), std::max(fabs(firstValue), fabs(secondValue)));
    return DIFFERENCE_VALUE <= std::max(absTol<F>(), relTol<F>() * SCALE_VALUE);
}
