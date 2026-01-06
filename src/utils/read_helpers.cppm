module;

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <type_traits>

export module utils.read_helpers;

import scan.types;
import value.flags;
import value;

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

// Try to read old value from Value* (strictly checks flags and length)
export template <typename T>
[[nodiscard]] inline auto oldValueAs(const Value* valuePtr) noexcept
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
    T out{};
    std::memcpy(&out, valuePtr->data(), sizeof(T));
    return out;
}