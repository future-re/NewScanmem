module;

#include <cstdint>
#include <cstring>
#include <bit>
#include <type_traits>
#include <concepts>
#include <variant>

export module endianness;

import value;

export namespace endianness {

// Concept for swappable integral types
template<typename T>
concept SwappableIntegral = std::integral<T> && (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

// Host endianness detection using compile-time constant
constexpr bool is_big_endian() noexcept {
    return std::endian::native == std::endian::big;
}

constexpr bool is_little_endian() noexcept {
    return std::endian::native == std::endian::little;
}

// Byte swapping functions
constexpr uint8_t swap_bytes(uint8_t value) noexcept {
    return value;
}

constexpr uint16_t swap_bytes(uint16_t value) noexcept {
    return static_cast<uint16_t>((value << 8) | (value >> 8));
}

constexpr uint32_t swap_bytes(uint32_t value) noexcept {
    return ((value & 0xFF000000u) >> 24) |
           ((value & 0x00FF0000u) >> 8) |
           ((value & 0x0000FF00u) << 8) |
           ((value & 0x000000FFu) << 24);
}

constexpr uint64_t swap_bytes(uint64_t value) noexcept {
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
template<typename T>
constexpr T swap_bytes_integral(T value) noexcept {
    static_assert(std::is_integral_v<T>, "T must be an integral type");
    
    if constexpr (sizeof(T) == 1) {
        return static_cast<T>(swap_bytes(static_cast<uint8_t>(value)));
    } else if constexpr (sizeof(T) == 2) {
        return static_cast<T>(swap_bytes(static_cast<uint16_t>(value)));
    } else if constexpr (sizeof(T) == 4) {
        return static_cast<T>(swap_bytes(static_cast<uint32_t>(value)));
    } else if constexpr (sizeof(T) == 8) {
        return static_cast<T>(swap_bytes(static_cast<uint64_t>(value)));
    } else {
        static_assert(sizeof(T) <= 8, "Unsupported integer size");
        return value;
    }
}

// Swap bytes in-place for variable-sized data
void swap_bytes_inplace(void* data, size_t size) {
    if (data == nullptr) return;
    
    switch (size) {
        case sizeof(uint16_t): {
            auto* ptr = static_cast<uint16_t*>(data);
            *ptr = swap_bytes(*ptr);
            break;
        }
        case sizeof(uint32_t): {
            auto* ptr = static_cast<uint32_t*>(data);
            *ptr = swap_bytes(*ptr);
            break;
        }
        case sizeof(uint64_t): {
            auto* ptr = static_cast<uint64_t*>(data);
            *ptr = swap_bytes(*ptr);
            break;
        }
        default:
            // Only support 2, 4, 8 byte swaps
            break;
    }
}

// Fix endianness for Value type based on flags
void fix_endianness(Value& value, bool reverse_endianness) noexcept {
    if (!reverse_endianness) {
        return;
    }

    // Use std::visit to handle the variant safely
    std::visit([&reverse_endianness](auto& val) {
        if constexpr (std::integral<std::decay_t<decltype(val)>>) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (sizeof(T) == 2) {
                val = static_cast<T>(swap_bytes(static_cast<uint16_t>(val)));
            } else if constexpr (sizeof(T) == 4) {
                val = static_cast<T>(swap_bytes(static_cast<uint32_t>(val)));
            } else if constexpr (sizeof(T) == 8) {
                val = static_cast<T>(swap_bytes(static_cast<uint64_t>(val)));
            }
        }
        // Non-integral types (float, double, arrays) are not swapped
    }, value.value);
}

// Convert between host and network byte order (big-endian)
template<SwappableIntegral T>
constexpr T host_to_network(T value) noexcept {
    if constexpr (is_big_endian()) {
        return value;
    } else {
        return swap_bytes(value);
    }
}

template<SwappableIntegral T>
constexpr T network_to_host(T value) noexcept {
    return host_to_network(value); // Same operation
}

// Convert between host and little-endian format
template<SwappableIntegral T>
constexpr T host_to_little_endian(T value) noexcept {
    if constexpr (is_little_endian()) {
        return value;
    } else {
        return swap_bytes(value);
    }
}

template<SwappableIntegral T>
constexpr T little_endian_to_host(T value) noexcept {
    return host_to_little_endian(value); // Same operation
}

} // namespace endianness