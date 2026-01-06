module;

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

export module ui.user_input;

import value;
import value.flags;

// Lightweight user input carrier:
// - Use Mem64 for underlying bytes (number/byte-array/string)
// - Use ScalarKind to tag number type/width (only in number/range modes)
// - Optional mask for byte matching (ByteArray + Mask)
export struct UserInput {
    ValueType valueType{MatchFlags::EMPTY};
    UserValue value;  // Primary value (number/bytes/string)

    UserInput() = default;

    // --- Convenience constructors (Number) ---
    template <typename T>
    static auto fromNumber(T val) -> UserInput {
        // TODO
        return UserInput{};
    }

    // --- Convenience constructors (RangeNumber) ---
    template <typename T>
    static auto fromRange(T loVal, T hiVal) -> UserInput {
        // TODO
        return UserInput{};
    }

    // --- Convenience constructors (Bytes) ---
    static auto fromBytes(std::span<const std::uint8_t> span) -> UserInput {
        // TODO
        return UserInput{};
    }

    // --- Convenience constructors (BytesWithMask) ---
    static auto fromBytesWithMask(std::span<const std::uint8_t> span,
                                  std::span<const std::uint8_t> mask)
        -> UserInput {
        // TODO
        return UserInput{};
    }

    // --- 构造便捷函数（String）---
    static auto fromString(const std::string& stringInput) -> UserInput {
        // TODO
        return UserInput{};
    }
};
