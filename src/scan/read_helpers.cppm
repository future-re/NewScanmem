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
import mem64;
import value; // 依赖项目中已有的 value 定义

// 本模块导出与字节读取、端序转换、TypeTraits 相关的辅助函数。
// 目标：为 numeric/bytes/string 模块提供统一的读取与类型标记工具。

// 端序条件交换（对整型/浮点/其他类型安全处理）
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

// 将 C++ 基础数值类型映射为 MatchFlags（用于保存/校验类型宽度）
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

// 安全读取指定类型并做端序转换
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

// 从 UserValue 中按类型提取低/高值（用于范围/比较）
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
[[nodiscard]] inline auto userValueHighAs(const UserValue& userValue) noexcept -> T {
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

// 尝试从 Value* 读取旧值（严格检查 flags 和长度）
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
    if (valuePtr->view().size() < sizeof(T)) {
        return std::nullopt;
    }
    T out{};
    std::memcpy(&out, valuePtr->view().data(), sizeof(T));
    return out;
}

// 浮点容差
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
