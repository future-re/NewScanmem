# Endianness Module Documentation

## Overview

The `endianness` module provides comprehensive byte order handling utilities for the NewScanmem project. It supports both compile-time and runtime endianness detection, byte swapping operations, and automatic endianness correction for various data types.

## Module Structure

```cpp
export module endianness;
```

## Dependencies

- `<cstdint>` - Fixed-width integer types
- `<cstring>` - C string operations
- `<bit>` - Bit operations and endian detection
- `<type_traits>` - Type traits for template metaprogramming
- `<concepts>` - C++20 concepts
- `<variant>` - Variant type support
- `value` module - Value type definitions

## Core Features

### 1. Endianness Detection

#### Compile-time Detection

```cpp
constexpr bool isBigEndian() noexcept;
constexpr bool isLittleEndian() noexcept;
```

Uses `std::endian::native` to determine host endianness at compile time.

### 2. Byte Swapping Functions

#### Basic Byte Swapping

```cpp
constexpr uint8_t swapBytes(uint8_t value) noexcept;
constexpr uint16_t swapBytes(uint16_t value) noexcept;
constexpr uint32_t swapBytes(uint32_t value) noexcept;
constexpr uint64_t swapBytes(uint64_t value) noexcept;
```

#### Generic Byte Swapping

```cpp
template<typename T>
constexpr T swapBytesIntegral(T value) noexcept;
```

Supports integer types of sizes 1, 2, 4, and 8 bytes.

### 3. Endianness Correction for Value Types

```cpp
void fixEndianness(Value& value, bool reverseEndianness) noexcept;
```

Automatically handles endianness correction for integral types stored in `Value` variants.

### 4. Network Byte Order Conversion

```cpp
template<SwappableIntegral T>
constexpr T hostToNetwork(T value) noexcept;

template<SwappableIntegral T>
constexpr T networkToHost(T value) noexcept;
```

Converts between host and network byte order (big-endian).

### 5. Little-endian Conversion

```cpp
template<SwappableIntegral T>
constexpr T hostToLittleEndian(T value) noexcept;

template<SwappableIntegral T>
constexpr T littleEndianToHost(T value) noexcept;
```

## Usage Examples

### Examples Basic Byte Swapping

```cpp
import endianness;

uint32_t value = 0x12345678;
uint32_t swapped = endianness::swapBytes(value);
// swapped = 0x78563412 on little-endian systems
```

### Endianness Correction

```cpp
import endianness;
import value;

Value val = uint32_t{0x12345678};
endianness::fixEndianness(val, true);  // Reverse endianness
```

### Network Communication

```cpp
uint16_t port = 8080;
uint16_t networkPort = endianness::hostToNetwork(port);
```

## Concepts and Constraints

### SwappableIntegral Concept

```cpp
template<typename T>
concept SwappableIntegral = std::integral<T> && 
    (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);
```

Restricts byte swapping operations to integral types of specific sizes.

## Implementation Details

### Byte Swapping Algorithms

- **16-bit**: Uses bit rotation: `(value << 8) | (value >> 8)`
- **32-bit**: Uses bit masking and shifting for optimal performance
- **64-bit**: Uses bit masking and shifting across 8 bytes

### Compile-time Optimization

All byte swapping operations are marked `constexpr` for compile-time evaluation when possible.

### Type Safety

Uses C++20 concepts to ensure type safety and provide clear error messages for unsupported types.

## Error Handling

- `swapBytesIntegral` uses `static_assert` for compile-time type checking
- `swapBytesInPlace` silently ignores unsupported sizes
- `fixEndianness` safely handles variant types using `std::visit`

## Performance Considerations

- All operations are constexpr for compile-time optimization
- Byte swapping uses efficient bit manipulation operations
- No dynamic memory allocation
- Minimal runtime overhead for supported types

## See Also

- [Value Module](value.md) - For Value type definitions
- [Target Memory Module](target_mem.md) - For memory analysis operations
