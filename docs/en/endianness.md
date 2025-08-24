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
constexpr bool is_big_endian() noexcept;
constexpr bool is_little_endian() noexcept;
```

Uses `std::endian::native` to determine host endianness at compile time.

### 2. Byte Swapping Functions

#### Basic Byte Swapping

```cpp
constexpr uint8_t swap_bytes(uint8_t value) noexcept;
constexpr uint16_t swap_bytes(uint16_t value) noexcept;
constexpr uint32_t swap_bytes(uint32_t value) noexcept;
constexpr uint64_t swap_bytes(uint64_t value) noexcept;
```

#### Generic Byte Swapping

```cpp
template<typename T>
constexpr T swap_bytes_integral(T value) noexcept;
```

Supports integer types of sizes 1, 2, 4, and 8 bytes.

#### In-place Byte Swapping

```cpp
void swap_bytes_inplace(void* data, size_t size);
```

Performs byte swapping in-place for 2, 4, or 8-byte data.

### 3. Endianness Correction for Value Types

```cpp
void fix_endianness(Value& value, bool reverse_endianness) noexcept;
```

Automatically handles endianness correction for integral types stored in `Value` variants.

### 4. Network Byte Order Conversion

```cpp
template<SwappableIntegral T>
constexpr T host_to_network(T value) noexcept;

template<SwappableIntegral T>
constexpr T network_to_host(T value) noexcept;
```

Converts between host and network byte order (big-endian).

### 5. Little-endian Conversion

```cpp
template<SwappableIntegral T>
constexpr T host_to_little_endian(T value) noexcept;

template<SwappableIntegral T>
constexpr T little_endian_to_host(T value) noexcept;
```

## Usage Examples

### Examples Basic Byte Swapping

```cpp
import endianness;

uint32_t value = 0x12345678;
uint32_t swapped = endianness::swap_bytes(value);
// swapped = 0x78563412 on little-endian systems
```

### Endianness Correction

```cpp
import endianness;
import value;

Value val = uint32_t{0x12345678};
endianness::fix_endianness(val, true);  // Reverse endianness
```

### Network Communication

```cpp
uint16_t port = 8080;
uint16_t network_port = endianness::host_to_network(port);
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

- `swap_bytes_integral` uses `static_assert` for compile-time type checking
- `swap_bytes_inplace` silently ignores unsupported sizes
- `fix_endianness` safely handles variant types using `std::visit`

## Performance Considerations

- All operations are constexpr for compile-time optimization
- Byte swapping uses efficient bit manipulation operations
- No dynamic memory allocation
- Minimal runtime overhead for supported types

## See Also

- [Value Module](value.md) - For Value type definitions
- [Target Memory Module](target_mem.md) - For memory analysis operations
