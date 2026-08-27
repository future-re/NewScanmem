#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

#include "newscanmem/value/core.hpp"
#include "newscanmem/value/flags.hpp"

// Lightweight UI-facing wrapper around the canonical scan operand model.
struct UserInput {
    ValueType valueType{MatchFlags::EMPTY};
    UserValue value;  // Primary value (number/bytes/string)

    UserInput() = default;

    // --- Convenience constructors (Number) ---
    template <typename T>
    static auto fromNumber(T val) -> UserInput {
        UserInput input;
        input.value = UserValue::fromScalar<T>(val);
        input.valueType = input.value.flag();
        return input;
    }

    // --- Convenience constructors (RangeNumber) ---
    template <typename T>
    static auto fromRange(T loVal, T hiVal) -> UserInput {
        UserInput input;
        input.value = UserValue{Value::of<T>(loVal), Value::of<T>(hiVal)};
        input.valueType = input.value.flag();
        return input;
    }

    // --- Convenience constructors (Bytes) ---
    static auto fromBytes(std::span<const std::uint8_t> span) -> UserInput;

    // --- Convenience constructors (BytesWithMask) ---
    static auto fromBytesWithMask(std::span<const std::uint8_t> span,
                                  std::span<const std::uint8_t> mask)
        -> UserInput;

    // --- 构造便捷函数（String）---
    static auto fromString(const std::string& stringInput) -> UserInput;
};
