module;

#include <bit>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <variant>

export module endianness;

import value;

// Concept for swappable integral types
template <std::size_t N>
constexpr bool IS_SWAPPABLE_SIZE = (N == 1) || (N == 2) || (N == 4) || (N == 8);

template <typename T>
concept SwappableIntegral = std::integral<T> && IS_SWAPPABLE_SIZE<sizeof(T)>;

export {
    // 大端字节序
    constexpr auto isBigEndian() noexcept -> bool {
        return std::endian::native == std::endian::big;
    }

    // 小端字节序
    constexpr auto isLittleEndian() noexcept -> bool {
        return std::endian::native == std::endian::little;
    }
}

// Byte swapping functions
constexpr auto swapBytes(uint8_t value) noexcept -> uint8_t { return value; }

/**
 * @brief Swaps the byte order of a 16-bit unsigned integer.
 *
 * @details 交换16位无符号整数的字节顺序。
 * 该函数接收一个16位无符号整数，并反转其字节顺序。
 * 主要用于在大端和小端表示之间进行数据转换。
 *
 * @param value 需要交换字节顺序的16位无符号整数。
 * @return 返回字节顺序已反转的16位无符号整数。
 *
 * @note 此函数被标记为`constexpr`和`noexcept`，意味着它可以在编译时求值，
 *       并且不会抛出异常。
 */
constexpr auto swapBytes(uint16_t value) noexcept -> uint16_t {
    return static_cast<uint16_t>((value << 8) | (value >> 8));
}

/**
 * @brief 交换 32 位无符号整数的字节顺序。
 *
 * 此函数将一个 32 位无符号整数的字节顺序从小端转换为大端，
 * 或从大端转换为小端。它通过递归调用 16 位版本的 swapBytes
 * 函数来实现字节交换。
 *
 * @param value 要交换字节顺序的 32 位无符号整数。
 * @return uint32_t 字节顺序已交换的 32 位无符号整数。
 */
constexpr auto swapBytes(uint32_t value) noexcept -> uint32_t {
    uint32_t res = swapBytes(static_cast<uint16_t>(value & 0xFFFF));
    res <<= 16;
    res |= swapBytes(static_cast<uint16_t>((value >> 16) & 0xFFFF));
    return res;
}

/**
 * @brief 交换 64 位无符号整数的字节顺序。
 *
 * 此函数通过交换输入的 64 位值的字节来进行字节序转换。
 * 它递归使用 swapBytes 的 32 位重载来分别处理低 32 位和高 32 位，
 * 然后将它们组合起来。
 *
 * @param value 要交换字节顺序的 64 位无符号整数。
 * @return 字节顺序已交换的 64 位无符号整数。
 *
 * @note 此函数是 constexpr 和 noexcept 的，允许在编译时求值，
 *       并确保不会抛出异常。
 */
constexpr auto swapBytes(uint64_t value) noexcept -> uint64_t {
    uint64_t res = swapBytes(static_cast<uint32_t>(value & 0xFFFFFFFF));
    res <<= 32;
    res |= swapBytes(static_cast<uint32_t>((value >> 32) & 0xFFFFFFFF));
    return res;
}

export {
    // 通用转换函数
    // Generic byte swapping for integral types
    template <typename T>
    constexpr auto swapBytesIntegral(T value) noexcept -> T {
        static_assert(std::is_integral_v<T>, "T must be an integral type");

        if constexpr (sizeof(T) == 1) {
            return static_cast<T>(swapBytes(static_cast<uint8_t>(value)));
        } else if constexpr (sizeof(T) == 2) {
            return static_cast<T>(swapBytes(static_cast<uint16_t>(value)));
        } else if constexpr (sizeof(T) == 4) {
            return static_cast<T>(swapBytes(static_cast<uint32_t>(value)));
        } else if constexpr (sizeof(T) == 8) {
            return static_cast<T>(swapBytes(static_cast<uint64_t>(value)));
        } else {
            static_assert(sizeof(T) <= 8, "Unsupported integer size");
            return value;
        }
    }

    // Fix endianness for Value type based on flags
    void fixEndianness(Value& value, bool reverseEndianness) noexcept {
        if (!reverseEndianness) {
            return;
        }
        if ((value.flags & MatchFlags::B64) != MatchFlags::EMPTY) {
            value.value = swapBytesIntegral(std::get<uint64_t>(value.value));
        } else if ((value.flags & MatchFlags::B32) != MatchFlags::EMPTY) {
            value.value = swapBytesIntegral(std::get<uint32_t>(value.value));
        } else if ((value.flags & MatchFlags::B16) != MatchFlags::EMPTY) {
            value.value = swapBytesIntegral(std::get<uint16_t>(value.value));
        }
    }

    // Convert between host and network byte order (big-endian)
    template <SwappableIntegral T>
    constexpr auto hostToNetwork(T value) noexcept -> T {
        if constexpr (isBigEndian()) {
            return value;
        } else {
            return swapBytes(value);
        }
    }

    template <SwappableIntegral T>
    constexpr auto networkToHost(T value) noexcept -> T {
        return hostToNetwork(value);  // Same operation
    }

    // Convert between host and little-endian format
    template <SwappableIntegral T>
    constexpr auto hostToLittleEndian(T value) noexcept -> T {
        if constexpr (isLittleEndian()) {
            return value;
        } else {
            return swapBytes(value);
        }
    }

    template <SwappableIntegral T>
    constexpr auto littleEndianToHost(T value) noexcept -> T {
        return hostToLittleEndian(value);  // Same operation
    }
}
