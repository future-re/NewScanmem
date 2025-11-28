module;

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

export module ui.user_input;

import utils.mem64;
import value.scalar; // ScalarKind helpers

// Lightweight user input carrier:
// - Use Mem64 for underlying bytes (number/byte-array/string)
// - Use ScalarKind to tag number type/width (only in number/range modes)
// - Optional mask for byte matching (ByteArray + Mask)
export struct UserInput {
    enum class Kind : std::uint8_t {
        NUMBER,
        BYTES,
        STRING,
        RANGE_NUMBER,
        BYTES_WITH_MASK
    };

    Kind kind{Kind::BYTES};
    ScalarKind numberKind{ScalarKind::U8};
    Mem64 value;                     // Primary value (number/bytes/string)
    std::optional<Mem64> highValue;  // Upper bound for numeric range (RangeNumber)
    std::optional<std::vector<std::uint8_t>> mask;  // Used by BytesWithMask

    UserInput() = default;

    // --- Convenience constructors (Number) ---
    template <typename T>
    static auto fromNumber(T val) -> UserInput {
        static_assert(std::is_trivially_copyable_v<T>,
                      "fromNumber requires trivially copyable T");
        UserInput userInput;
        userInput.kind = Kind::NUMBER;
        userInput.numberKind = KindOf<T>::VALUE;
        userInput.value.setScalar<T>(val);
        return userInput;
    }

    // --- Convenience constructors (RangeNumber) ---
    template <typename T>
    static auto fromRange(T loVal, T hiVal) -> UserInput {
        static_assert(std::is_trivially_copyable_v<T>,
                      "fromRange requires trivially copyable T");
        UserInput userInput;
        userInput.kind = Kind::RANGE_NUMBER;
        userInput.numberKind = KindOf<T>::VALUE;
        userInput.value.setScalar<T>(loVal);
        userInput.highValue.emplace();
        userInput.highValue->setScalar<T>(hiVal);
        return userInput;
    }

    // --- Convenience constructors (Bytes) ---
    static auto fromBytes(std::span<const std::uint8_t> span) -> UserInput {
        UserInput userInput;
        userInput.kind = Kind::BYTES;
        userInput.value = Mem64{span};
        return userInput;
    }

    // --- Convenience constructors (BytesWithMask) ---
    static auto fromBytesWithMask(std::span<const std::uint8_t> span,
                                  std::span<const std::uint8_t> mask)
        -> UserInput {
        UserInput userInput;
        userInput.kind = Kind::BYTES_WITH_MASK;
        userInput.value = Mem64{span};
        userInput.mask = std::vector<std::uint8_t>(mask.begin(), mask.end());
        return userInput;
    }

    // --- 构造便捷函数（String）---
    static auto fromString(const std::string& stringInput) -> UserInput {
        UserInput userInput;
        userInput.kind = Kind::STRING;
        userInput.value.setString(stringInput);
        return userInput;
    }
};
