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
template <typename T>
concept SwappableIntegral =
    std::integral<T> &&
    (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

// Host endianness detection using compile-time constant
constexpr bool isBigEndian() noexcept {
    return std::endian::native == std::endian::big;
}

constexpr bool isLittleEndian() noexcept {
    return std::endian::native == std::endian::little;
}

// Byte swapping functions
constexpr uint8_t swapBytes(uint8_t value) noexcept { return value; }

constexpr uint16_t swapBytes(uint16_t value) noexcept {
    return static_cast<uint16_t>((value << 8) | (value >> 8));
}

constexpr uint32_t swapBytes(uint32_t value) noexcept {
    return ((value & 0xFF000000u) >> 24) | ((value & 0x00FF0000u) >> 8) |
           ((value & 0x0000FF00u) << 8) | ((value & 0x000000FFu) << 24);
}

constexpr uint64_t swapBytes(uint64_t value) noexcept {
    return ((value & 0xFF00000000000000ull) >> 56) |
           ((value & 0x00FF000000000000ull) >> 40) |
           ((value & 0x0000FF0000000000ull) >> 24) |
           ((value & 0x000000FF00000000ull) >> 8) |
           ((value & 0x00000000FF000000ull) << 8) |
           ((value & 0x0000000000FF0000ull) << 24) |
           ((value & 0x000000000000FF00ull) << 40) |
           ((value & 0x00000000000000FFull) << 56);
}

// Generic byte swapping for integral types
template <typename T>
constexpr T swapBytesIntegral(T value) noexcept {
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

// Swap bytes in-place for variable-sized data
void swapBytesInPlace(void* data, size_t size) {
    if (data == nullptr) return;

    switch (size) {
        case sizeof(uint16_t): {
            auto* ptr = static_cast<uint16_t*>(data);
            *ptr = swapBytes(*ptr);
            break;
        }
        case sizeof(uint32_t): {
            auto* ptr = static_cast<uint32_t*>(data);
            *ptr = swapBytes(*ptr);
            break;
        }
        case sizeof(uint64_t): {
            auto* ptr = static_cast<uint64_t*>(data);
            *ptr = swapBytes(*ptr);
            break;
        }
        default:
            // Only support 2, 4, 8 byte swaps
            break;
    }
}

// Fix endianness for Value type based on flags
void fixEndianness(Value& value, bool reverseEndianness) noexcept {
    if (!reverseEndianness) {
        return;
    }

    // Use std::visit to handle the variant safely
    std::visit(
        [&reverseEndianness](auto& val) {
            if constexpr (std::integral<std::decay_t<decltype(val)>>) {
                using T = std::decay_t<decltype(val)>;
                if constexpr (sizeof(T) == 2) {
                    val = static_cast<T>(swapBytes(static_cast<uint16_t>(val)));
                } else if constexpr (sizeof(T) == 4) {
                    val = static_cast<T>(swapBytes(static_cast<uint32_t>(val)));
                } else if constexpr (sizeof(T) == 8) {
                    val = static_cast<T>(swapBytes(static_cast<uint64_t>(val)));
                }
            }
            // Non-integral types (float, double, arrays) are not swapped
        },
        value.value);
}

// Convert between host and network byte order (big-endian)
template <SwappableIntegral T>
constexpr T hostToNetwork(T value) noexcept {
    if constexpr (isBigEndian()) {
        return value;
    } else {
        return swapBytes(value);
    }
}

template <SwappableIntegral T>
constexpr T networkToHost(T value) noexcept {
    return hostToNetwork(value);  // Same operation
}

// Convert between host and little-endian format
template <SwappableIntegral T>
constexpr T hostToLittleEndian(T value) noexcept {
    if constexpr (isLittleEndian()) {
        return value;
    } else {
        return swapBytes(value);
    }
}

template <SwappableIntegral T>
constexpr T littleEndianToHost(T value) noexcept {
    return hostToLittleEndian(value);  // Same operation
}
