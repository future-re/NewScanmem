module;

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <optional>
#include <type_traits>

export module scan.read_helpers;

import scan.types;
import value; // 依赖项目中已有的 value 定义

// 本模块导出与字节读取、端序转换、TypeTraits 相关的辅助函数。
// 目标：为 numeric/bytes/string 模块提供统一的读取与类型标记工具。

namespace detail {

template <typename T>
struct TypeTraits;

template <>
struct TypeTraits<int8_t> {
    static constexpr MatchFlags FLAG = MatchFlags::S8B;
    static constexpr auto LOW = &UserValue::int8Value;
    static constexpr auto HIGH = &UserValue::int8RangeHighValue;
};

template <>
struct TypeTraits<uint8_t> {
    static constexpr MatchFlags FLAG = MatchFlags::U8B;
    static constexpr auto LOW = &UserValue::uint8Value;
    static constexpr auto HIGH = &UserValue::uint8RangeHighValue;
};

template <>
struct TypeTraits<int16_t> {
    static constexpr MatchFlags FLAG = MatchFlags::S16B;
    static constexpr auto LOW = &UserValue::int16Value;
    static constexpr auto HIGH = &UserValue::int16RangeHighValue;
};

template <>
struct TypeTraits<uint16_t> {
    static constexpr MatchFlags FLAG = MatchFlags::U16B;
    static constexpr auto LOW = &UserValue::uint16Value;
    static constexpr auto HIGH = &UserValue::uint16RangeHighValue;
};

template <>
struct TypeTraits<int32_t> {
    static constexpr MatchFlags FLAG = MatchFlags::S32B;
    static constexpr auto LOW = &UserValue::int32Value;
    static constexpr auto HIGH = &UserValue::int32RangeHighValue;
};

template <>
struct TypeTraits<uint32_t> {
    static constexpr MatchFlags FLAG = MatchFlags::U32B;
    static constexpr auto LOW = &UserValue::uint32Value;
    static constexpr auto HIGH = &UserValue::uint32RangeHighValue;
};

template <>
struct TypeTraits<int64_t> {
    static constexpr MatchFlags FLAG = MatchFlags::S64B;
    static constexpr auto LOW = &UserValue::int64Value;
    static constexpr auto HIGH = &UserValue::int64RangeHighValue;
};

template <>
struct TypeTraits<uint64_t> {
    static constexpr MatchFlags FLAG = MatchFlags::U64B;
    static constexpr auto LOW = &UserValue::uint64Value;
    static constexpr auto HIGH = &UserValue::uint64RangeHighValue;
};

template <>
struct TypeTraits<float> {
    static constexpr MatchFlags FLAG = MatchFlags::F32B;
    static constexpr auto LOW = &UserValue::float32Value;
    static constexpr auto HIGH = &UserValue::float32RangeHighValue;
};

template <>
struct TypeTraits<double> {
    static constexpr MatchFlags FLAG = MatchFlags::F64B;
    static constexpr auto LOW = &UserValue::float64Value;
    static constexpr auto HIGH = &UserValue::float64RangeHighValue;
};

}  // namespace detail

export template <typename T>
[[nodiscard]] constexpr auto flagForType() noexcept -> MatchFlags {
    return detail::TypeTraits<T>::FLAG;
}

export template <typename T>
[[nodiscard]] inline auto userValueAs(const UserValue& userValue) noexcept
    -> T {
    return userValue.*(detail::TypeTraits<T>::LOW);
}

export template <typename T>
[[nodiscard]] inline auto userValueHighAs(const UserValue& userValue) noexcept
    -> T {
    return userValue.*(detail::TypeTraits<T>::HIGH);
}

// 端序条件交换（对整型/浮点/其他类型安全处理）
export template <typename T>
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
        return value;
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
