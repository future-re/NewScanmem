module;

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>
#include <type_traits>

export module scan.read_helpers;

import scan.types;
import value.flags;
import utils.mem64;
import value; // Depends on the project's existing value definitions

// Helpers for byte reading, endianness conversion, and type traits
// Goal: provide unified reading and type-tagging utilities for numeric/bytes/string modules

// Conditional endianness swap (safe for integral/floating/other trivially copyable types)
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

// Map C++ fundamental types to MatchFlags (to record/verify bit width)
export template <typename T>
[[nodiscard]] constexpr auto flagForType() noexcept -> MatchFlags {
    if constexpr (std::is_same_v<T, float>) {
        return MatchFlags::B32;
    } else if constexpr (std::is_same_v<T, double>) {
        return MatchFlags::B64;
    } else if constexpr (std::is_integral_v<T>) {
        if constexpr (sizeof(T) == 1) {
            return MatchFlags::B8;
        } else if constexpr (sizeof(T) == 2) {
            return MatchFlags::B16;
        } else if constexpr (sizeof(T) == 4) {
            return MatchFlags::B32;
        } else if constexpr (sizeof(T) == 8) {
            return MatchFlags::B64;
        } else {
            return MatchFlags::EMPTY;
        }
    } else {
        return MatchFlags::EMPTY;
    }
}

// Safely read specified type and apply endianness conversion
export template <typename T>
[[nodiscard]] inline auto readTyped(const Mem64* memoryPtr, size_t memLength,
                                    bool reverseEndianness) noexcept
    -> std::optional<T> {
    if (memLength < sizeof(T)) {
        return std::nullopt;
    }
    auto currentOpt = memoryPtr->tryGet<T>();
    if (!currentOpt) {
        return std::nullopt;
    }
    T val = swapIfReverse<T>(*currentOpt, reverseEndianness);
    return val;
}

// Extract low/high values from UserValue by type (for range/compare)
export template <typename T>
[[nodiscard]] inline auto userValueAs(const UserValue& userValue) noexcept
    -> T {
    if constexpr (std::is_same_v<T, int8_t>) {
        return userValue.s8;
    } else if constexpr (std::is_same_v<T, uint8_t>) {
        return userValue.u8;
    } else if constexpr (std::is_same_v<T, int16_t>) {
        return userValue.s16;
    } else if constexpr (std::is_same_v<T, uint16_t>) {
        return userValue.u16;
    } else if constexpr (std::is_same_v<T, int32_t>) {
        return userValue.s32;
    } else if constexpr (std::is_same_v<T, uint32_t>) {
        return userValue.u32;
    } else if constexpr (std::is_same_v<T, int64_t>) {
        return userValue.s64;
    } else if constexpr (std::is_same_v<T, uint64_t>) {
        return userValue.u64;
    } else if constexpr (std::is_same_v<T, float>) {
        return userValue.f32;
    } else if constexpr (std::is_same_v<T, double>) {
        return userValue.f64;
    } else {
        static_assert(std::is_trivially_copyable_v<T>,
                      "Unsupported type for userValueAs<T>");
        return T{};
    }
}

export template <typename T>
[[nodiscard]] inline auto userValueHighAs(const UserValue& userValue) noexcept
    -> T {
    if constexpr (std::is_same_v<T, int8_t>) {
        return userValue.s8h;
    } else if constexpr (std::is_same_v<T, uint8_t>) {
        return userValue.u8h;
    } else if constexpr (std::is_same_v<T, int16_t>) {
        return userValue.s16h;
    } else if constexpr (std::is_same_v<T, uint16_t>) {
        return userValue.u16h;
    } else if constexpr (std::is_same_v<T, int32_t>) {
        return userValue.s32h;
    } else if constexpr (std::is_same_v<T, uint32_t>) {
        return userValue.u32h;
    } else if constexpr (std::is_same_v<T, int64_t>) {
        return userValue.s64h;
    } else if constexpr (std::is_same_v<T, uint64_t>) {
        return userValue.u64h;
    } else if constexpr (std::is_same_v<T, float>) {
        return userValue.f32h;
    } else if constexpr (std::is_same_v<T, double>) {
        return userValue.f64h;
    } else {
        static_assert(std::is_trivially_copyable_v<T>,
                      "Unsupported type for userValueHighAs<T>");
        return T{};
    }
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

// Floating-point tolerance
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
[[nodiscard]] inline auto almostEqual(F firstValue, F secondValue) noexcept
    -> bool {
    using std::fabs;
    const F DIFFERENCE_VALUE = fabs(firstValue - secondValue);
    const F SCALE_VALUE =
        std::max(F(1), std::max(fabs(firstValue), fabs(secondValue)));
    return DIFFERENCE_VALUE <= std::max(absTol<F>(), relTol<F>() * SCALE_VALUE);
}
