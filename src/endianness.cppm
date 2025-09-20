module;

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <span>
#include <stdexcept>
#include <type_traits>

export module endianness;

// 导出概念：支持 1/2/4/8 字节的可交换二进制类型（整数或浮点）
export template <typename T>
concept SwappableBinary =
    std::is_trivially_copyable_v<T> &&
    (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

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

// 通用字节反转函数（整数）
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

// 通用字节反转（泛型）：整数直接调用上面的实现，其他类型通过 bit_cast
template <SwappableBinary T>
constexpr auto swapBytesBinary(T value) noexcept -> T {
    if constexpr (std::is_integral_v<T>) {
        return swapBytesIntegral(value);
    } else {
        if constexpr (sizeof(T) == 1) {
            return value;
        } else if constexpr (sizeof(T) == 2) {
            auto uVal = std::bit_cast<uint16_t>(value);
            uVal = swapBytes(uVal);
            return std::bit_cast<T>(uVal);
        } else if constexpr (sizeof(T) == 4) {
            auto uVal = std::bit_cast<uint32_t>(value);
            uVal = swapBytes(uVal);
            return std::bit_cast<T>(uVal);
        } else if constexpr (sizeof(T) == 8) {
            auto uVal = std::bit_cast<uint64_t>(value);
            uVal = swapBytes(uVal);
            return std::bit_cast<T>(uVal);
        } else {
            return value;
        }
    }
}

// 字节流反转（内部使用）
void reverseByteStream(std::span<uint8_t> byteStream) {
    std::ranges::reverse(byteStream);
}

// ==========================
// 对外接口（导出）
// ==========================
// 将对外接口放入命名空间，与模块名一致，便于使用方以 endianness:: 方式调用
export namespace endianness {
// 标量值的字节序转换
template <SwappableBinary T>
constexpr auto hostToNetwork(T value) noexcept -> T {
    if constexpr (isBigEndian()) {
        return value;
    } else {
        return swapBytesBinary(value);
    }
}

template <SwappableBinary T>
constexpr auto networkToHost(T value) noexcept -> T {
    return hostToNetwork(value);  // Same operation
}

template <SwappableBinary T>
constexpr auto hostToLittleEndian(T value) noexcept -> T {
    if constexpr (isLittleEndian()) {
        return value;
    } else {
        return swapBytesBinary(value);
    }
}

template <SwappableBinary T>
constexpr auto littleEndianToHost(T value) noexcept -> T {
    return hostToLittleEndian(value);  // Same operation
}

// 字节流的字节序转换，toLitteEndian 为 true 表示转为小端，否则转为大端
template <SwappableBinary T>
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
}  // namespace endianness
