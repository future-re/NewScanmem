module;

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

export module user.input;

import mem64;
import value.scalar; // ScalarKind helpers

// 简洁的用户输入载体：
// - 用 Mem64 承载底层字节（数值/字节数组/字符串）
// - 用 ScalarKind 标注数值的类型/宽度（仅在数值/范围模式下使用）
// - 可选的 mask 用于字节匹配（ByteArray + Mask）
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
    Mem64 value;                     // 主值（数值/字节/字符串）
    std::optional<Mem64> highValue;  // 数值范围上界（RangeNumber 使用）
    std::optional<std::vector<std::uint8_t>> mask;  // BytesWithMask 使用

    UserInput() = default;

    // --- 构造便捷函数（Number）---
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

    // --- 构造便捷函数（RangeNumber）---
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

    // --- 构造便捷函数（Bytes）---
    static auto fromBytes(std::span<const std::uint8_t> span) -> UserInput {
        UserInput userInput;
        userInput.kind = Kind::BYTES;
        userInput.value = Mem64{span};
        return userInput;
    }

    // --- 构造便捷函数（BytesWithMask）---
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
