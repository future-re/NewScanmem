module;

#include <algorithm>
#include <bit>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <variant>

export module utils.endianness;

// Concept: Swappable binary types (integers or floats of size 1, 2, 4, 8)
export template <typename T>
concept SwappableBinary =
    std::is_trivially_copyable_v<T> &&
    (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);

// Concept: Check if a type is a std::variant
template <typename T>
struct IsVariant : std::false_type {};

template <typename... Ts>
struct IsVariant<std::variant<Ts...>> : std::true_type {};

template <typename T>
concept VariantType = IsVariant<std::remove_cvref_t<T>>::value;

// ==========================
// Internal Implementation
// ==========================

namespace detail {

// The core byte-swapping logic for fundamental integer types.
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

// Generic byte-swapping function for any SwappableBinary type using
// std::bit_cast.
template <SwappableBinary T>
constexpr auto swapBytesGeneric(T value) noexcept -> T {
    if constexpr (sizeof(T) == 1) {
        return value;
    } else if constexpr (sizeof(T) == 2) {
        return std::bit_cast<T>(swapBytes(std::bit_cast<uint16_t>(value)));
    } else if constexpr (sizeof(T) == 4) {
        return std::bit_cast<T>(swapBytes(std::bit_cast<uint32_t>(value)));
    } else if constexpr (sizeof(T) == 8) {
        return std::bit_cast<T>(swapBytes(std::bit_cast<uint64_t>(value)));
    }
    return value;
}

// In-place byte swap for a span of bytes.
void swapBytesInPlace(std::span<uint8_t> bytes) { std::ranges::reverse(bytes); }

}  // namespace detail

// ==========================
// Public API
// ==========================

export namespace endianness {

// Determines if a byte swap is needed to convert from native to target
// endianness.
template <std::endian Target>
constexpr auto needSwap() noexcept -> bool {
    return std::endian::native != Target;
}

// --- Scalar Conversions ---

template <SwappableBinary T>
constexpr auto hostToNetwork(T value) noexcept -> T {
    if constexpr (needSwap<std::endian::big>()) {
        return detail::swapBytesGeneric(value);
    }
    return value;
}

template <SwappableBinary T>
constexpr auto networkToHost(T value) noexcept -> T {
    return hostToNetwork(value);  // Same operation
}

template <SwappableBinary T>
constexpr auto hostToLittleEndian(T value) noexcept -> T {
    if constexpr (needSwap<std::endian::little>()) {
        return detail::swapBytesGeneric(value);
    }
    return value;
}

template <SwappableBinary T>
constexpr auto littleEndianToHost(T value) noexcept -> T {
    return hostToLittleEndian(value);  // Same operation
}

// --- In-place Byte Stream Conversions (uniform element size) ---

template <SwappableBinary T>
void hostToNetwork(std::span<uint8_t> byteStream) {
    if (needSwap<std::endian::big>()) {
        const size_t TYPESIZE = sizeof(T);
        if (byteStream.size() % TYPESIZE != 0) {
            throw std::invalid_argument(
                "Byte stream size must be a multiple of type size");
        }
        for (size_t i = 0; i < byteStream.size(); i += TYPESIZE) {
            detail::swapBytesInPlace(byteStream.subspan(i, TYPESIZE));
        }
    }
}

template <SwappableBinary T>
void networkToHost(std::span<uint8_t> byteStream) {
    hostToNetwork<T>(byteStream);  // Same operation
}

template <SwappableBinary T>
void hostToLittleEndian(std::span<uint8_t> byteStream) {
    if (needSwap<std::endian::little>()) {
        const size_t TYPESIZE = sizeof(T);
        if (byteStream.size() % TYPESIZE != 0) {
            throw std::invalid_argument(
                "Byte stream size must be a multiple of type size");
        }
        for (size_t i = 0; i < byteStream.size(); i += TYPESIZE) {
            detail::swapBytesInPlace(byteStream.subspan(i, TYPESIZE));
        }
    }
}

template <SwappableBinary T>
void littleEndianToHost(std::span<uint8_t> byteStream) {
    hostToLittleEndian<T>(byteStream);  // Same operation
}

// --- In-place Byte Stream Conversions (structured layout) ---

namespace detail {
// Helper for structured conversion to avoid duplicating logic
void convertStructured(std::span<uint8_t> byteStream,
                       std::initializer_list<size_t> layout) {
    size_t offset = 0;
    for (size_t elementSize : layout) {
        if (offset + elementSize > byteStream.size()) {
            throw std::invalid_argument(
                "Byte stream size does not match layout");
        }
        // Only swap for multi-byte elements
        if (elementSize > 1) {
            ::detail::swapBytesInPlace(byteStream.subspan(offset, elementSize));
        }
        offset += elementSize;
    }
    if (offset != byteStream.size()) {
        throw std::invalid_argument(
            "Layout does not cover the entire byte stream");
    }
}
}  // namespace detail

void hostToNetwork(std::span<uint8_t> byteStream,
                   std::initializer_list<size_t> layout) {
    if (needSwap<std::endian::big>()) {
        detail::convertStructured(byteStream, layout);
    }
}

void networkToHost(std::span<uint8_t> byteStream,
                   std::initializer_list<size_t> layout) {
    hostToNetwork(byteStream, layout);  // Same operation
}

void hostToLittleEndian(std::span<uint8_t> byteStream,
                        std::initializer_list<size_t> layout) {
    if (needSwap<std::endian::little>()) {
        detail::convertStructured(byteStream, layout);
    }
}

void littleEndianToHost(std::span<uint8_t> byteStream,
                        std::initializer_list<size_t> layout) {
    hostToLittleEndian(byteStream, layout);  // Same operation
}

// --- In-place Variant Conversion ---

template <VariantType Variant>
void hostToNetwork(Variant& var) {
    std::visit(
        [](auto& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (SwappableBinary<T>) {
                val = hostToNetwork(val);
            }
        },
        var);
}

template <VariantType Variant>
void networkToHost(Variant& var) {
    hostToNetwork(var);  // Same operation
}

template <VariantType Variant>
void hostToLittleEndian(Variant& var) {
    std::visit(
        [](auto& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (SwappableBinary<T>) {
                val = hostToLittleEndian(val);
            }
        },
        var);
}

template <VariantType Variant>
void littleEndianToHost(Variant& var) {
    hostToLittleEndian(var);  // Same operation
}

}  // namespace endianness

// Re-export common APIs for unqualified access
export using endianness::hostToNetwork;
export using endianness::networkToHost;
export using endianness::hostToLittleEndian;
export using endianness::littleEndianToHost;

// Exported utility: swap bytes for integral types (1/2/4/8 bytes)
export template <typename T>
constexpr auto swapBytesIntegral(T value) noexcept -> T {
    static_assert(std::is_integral_v<T>, "T must be an integral type");
    return ::detail::swapBytesGeneric(value);
}

// Export for testing or utility purposes
export constexpr auto isBigEndian() noexcept -> bool {
    return std::endian::native == std::endian::big;
}

export constexpr auto isLittleEndian() noexcept -> bool {
    return std::endian::native == std::endian::little;
}
