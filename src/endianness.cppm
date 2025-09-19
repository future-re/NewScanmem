module;

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <span>
#include <stdexcept>
#include <type_traits>

export module endianness;

// Concept for swappable integral types
template <std::size_t N>
constexpr bool IS_SWAPPABLE_SIZE = (N == 1) || (N == 2) || (N == 4) || (N == 8);

template <typename T>
concept SwappableIntegral = std::integral<T> && IS_SWAPPABLE_SIZE<sizeof(T)>;

// ==========================
// 内部工具函数（不导出）
// ==========================

// 判断主机字节序
constexpr auto isBigEndian() noexcept -> bool {
    return std::endian::native == std::endian::big;
}

constexpr auto isLittleEndian() noexcept -> bool {
    return std::endian::native == std::endian::little;
}

// 字节反转函数（内部使用）
constexpr auto swapBytes(uint8_t value) noexcept -> uint8_t { return value; }

constexpr auto swapBytes(uint16_t value) noexcept -> uint16_t {
    return static_cast<uint16_t>((value << 8) | (value >> 8));
}

constexpr auto swapBytes(uint32_t value) noexcept -> uint32_t {
    uint32_t res = swapBytes(static_cast<uint16_t>(value & 0xFFFF));
    res <<= 16;
    res |= swapBytes(static_cast<uint16_t>((value >> 16) & 0xFFFF));
    return res;
}

constexpr auto swapBytes(uint64_t value) noexcept -> uint64_t {
    uint64_t res = swapBytes(static_cast<uint32_t>(value & 0xFFFFFFFF));
    res <<= 32;
    res |= swapBytes(static_cast<uint32_t>((value >> 32) & 0xFFFFFFFF));
    return res;
}

// 通用字节反转函数
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

// 字节流反转（内部使用）
void reverseByteStream(std::span<uint8_t> byteStream) {
    std::ranges::reverse(byteStream);
}

// ==========================
// 对外接口（导出）
// ==========================
export {
    // 标量值的字节序转换
    template <SwappableIntegral T>
    constexpr auto hostToNetwork(T value) noexcept -> T {
        if constexpr (isBigEndian()) {
            return value;
        } else {
            return swapBytesIntegral(value);
        }
    }

    template <SwappableIntegral T>
    constexpr auto networkToHost(T value) noexcept -> T {
        return hostToNetwork(value);  // Same operation
    }

    template <SwappableIntegral T>
    constexpr auto hostToLittleEndian(T value) noexcept -> T {
        if constexpr (isLittleEndian()) {
            return value;
        } else {
            return swapBytesIntegral(value);
        }
    }

    template <SwappableIntegral T>
    constexpr auto littleEndianToHost(T value) noexcept -> T {
        return hostToLittleEndian(value);  // Same operation
    }

    // 字节流的字节序转换，toLitteEndian 为 true 表示转为小端，否则转为大端
    template <SwappableIntegral T>
    void convertEndian(std::span<uint8_t> byteStream, bool toLittleEndian) {
        const size_t TYPE_SIZE = sizeof(T);
        if (byteStream.size() % TYPE_SIZE != 0) {
            throw std::invalid_argument(
                "Byte stream size must be a multiple of type size");
        }

        for (size_t i = 0; i < byteStream.size(); i += TYPE_SIZE) {
            T value;
            std::memcpy(&value, &byteStream[i], TYPE_SIZE);

            if (toLittleEndian) {
                value = hostToLittleEndian(value);
            } else {
                value = hostToNetwork(value);  // Big-endian
            }

            std::memcpy(&byteStream[i], &value, TYPE_SIZE);
        }
    }

    // 字节流的字节序转换，按指定规则解析, toLittleEndian 为 true
    // 表示转为小端，否则转为大端
    void convertEndian(std::span<uint8_t> byteStream, bool toLittleEndian,
                       std::initializer_list<size_t> parseRules = {}) {
        if (parseRules.size() == 0) {
            // 如果没有解析规则，直接反转整个字节流
            reverseByteStream(byteStream);
            return;
        }

        size_t offset = 0;
        for (size_t elementSize : parseRules) {
            if (offset + elementSize > byteStream.size()) {
                throw std::invalid_argument(
                    "Byte stream size does not match parse rules");
            }

            std::span<uint8_t> element(byteStream.subspan(offset, elementSize));

            // 如果目标是 Little Endian 且主机是 Big
            // Endian，或者反之，则反转每个元素
            if ((toLittleEndian && isBigEndian()) ||
                (!toLittleEndian && isLittleEndian())) {
                reverseByteStream(element);
            }

            offset += elementSize;
        }

        if (offset != byteStream.size()) {
            throw std::invalid_argument(
                "Parse rules do not cover the entire byte stream");
        }
    }
}