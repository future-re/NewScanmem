module;

#include <bit>
#include <cstdint>
#include <type_traits>

export module utils.endianness;

namespace detail {

// Core byte-swap primitives
constexpr auto swapBytes(uint8_t value) noexcept -> uint8_t { return value; }

constexpr auto swapBytes(uint16_t value) noexcept -> uint16_t {
    return static_cast<uint16_t>((value << 8) | (value >> 8));
}

constexpr auto swapBytes(uint32_t value) noexcept -> uint32_t {
    return (static_cast<uint32_t>(swapBytes(static_cast<uint16_t>(value)))
            << 16) |
           swapBytes(static_cast<uint16_t>(value >> 16));
}

constexpr auto swapBytes(uint64_t value) noexcept -> uint64_t {
    return (static_cast<uint64_t>(swapBytes(static_cast<uint32_t>(value)))
            << 32) |
           swapBytes(static_cast<uint32_t>(value >> 32));
}

}  // namespace detail

export namespace utils {

// Target endianness type
enum class Endianness { LITTLE, BIG };

// Generic byte-swap for integral/floating types
template <typename T>
constexpr auto swapBytes(T value) noexcept -> T {
    static_assert(std::is_trivially_copyable_v<T>,
                  "T must be trivially copyable");
    static_assert(
        sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8,
        "T must be 1, 2, 4, or 8 bytes");

    if constexpr (sizeof(T) == 1) {
        return value;
    } else if constexpr (sizeof(T) == 2) {
        return std::bit_cast<T>(
            detail::swapBytes(std::bit_cast<uint16_t>(value)));
    } else if constexpr (sizeof(T) == 4) {
        return std::bit_cast<T>(
            detail::swapBytes(std::bit_cast<uint32_t>(value)));
    } else if constexpr (sizeof(T) == 8) {
        return std::bit_cast<T>(
            detail::swapBytes(std::bit_cast<uint64_t>(value)));
    }
}

// Host <-> Network (big-endian)
template <typename T>
constexpr auto hostToNetwork(T value) noexcept -> T {
    if constexpr (std::endian::native == std::endian::big) {
        return value;
    } else {
        return swapBytes(value);
    }
}

template <typename T>
constexpr auto networkToHost(T value) noexcept -> T {
    return hostToNetwork(value);  // symmetric operation
}

// Host <-> Little-endian
template <typename T>
constexpr auto hostToLittleEndian(T value) noexcept -> T {
    if constexpr (std::endian::native == std::endian::little) {
        return value;
    } else {
        return swapBytes(value);
    }
}

template <typename T>
constexpr auto littleEndianToHost(T value) noexcept -> T {
    return hostToLittleEndian(value);  // symmetric operation
}

// Endianness query helpers
constexpr auto isBigEndian() noexcept -> bool {
    return std::endian::native == std::endian::big;
}

constexpr auto isLittleEndian() noexcept -> bool {
    return std::endian::native == std::endian::little;
}

template <typename T>
constexpr auto swapBytesIntegral(T value) noexcept -> T {
    static_assert(std::is_integral_v<T>, "T must be an integral type");
    return swapBytes(value);
}

// Normalize value to target endianness relative to host
template <typename T>
constexpr auto toTargetEndian(T value, Endianness target) noexcept -> T {
    if (target == Endianness::LITTLE) {
        return hostToLittleEndian(value);
    }
    // target big-endian
    if constexpr (std::endian::native == std::endian::big) {
        return value;
    } else {
        return swapBytes(value);
    }
}

auto getHost() -> Endianness {
    if (isLittleEndian()) {
        return Endianness::LITTLE;
    }
    return Endianness::BIG;
}

}  // namespace utils
