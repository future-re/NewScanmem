module;

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

export module ui.user_input;

import value.core;
import value.flags;

// Lightweight UI-facing wrapper around the canonical scan operand model.
export struct UserInput {
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
        input.value =
            UserValue{Value::of<T>(loVal), Value::of<T>(hiVal)};
        input.valueType = input.value.flag();
        return input;
    }

    // --- Convenience constructors (Bytes) ---
    static auto fromBytes(std::span<const std::uint8_t> span) -> UserInput {
        UserInput input;
        input.value = UserValue::fromByteArray(
            std::vector<std::uint8_t>(span.begin(), span.end()));
        input.valueType = MatchFlags::BYTE_ARRAY;
        return input;
    }

    // --- Convenience constructors (BytesWithMask) ---
    static auto fromBytesWithMask(std::span<const std::uint8_t> span,
                                  std::span<const std::uint8_t> mask)
        -> UserInput {
        UserInput input;
        input.value = UserValue::fromByteArray(
            std::vector<std::uint8_t>(span.begin(), span.end()),
            std::vector<std::uint8_t>(mask.begin(), mask.end()));
        input.valueType = MatchFlags::BYTE_ARRAY;
        return input;
    }

    // --- 构造便捷函数（String）---
    static auto fromString(const std::string& stringInput) -> UserInput {
        UserInput input;
        input.value = UserValue::fromString(stringInput);
        input.valueType = MatchFlags::STRING;
        return input;
    }
};
