# Value Module Documentation

## Overview

The `value` module provides comprehensive value type definitions and match flags for the NewScanmem project. It defines various data type representations, memory layouts, and utility structures for handling different numeric types, byte arrays, and wildcard patterns used in memory scanning operations.

## Module Structure

```cpp
export module value;
```

## Dependencies

- `<array>` - Fixed-size array container
- `<bit>` - Bit operations
- `<cstdint>` - Fixed-width integer types
- `<optional>` - Optional type support
- `<string>` - String operations
- `<variant>` - Variant type support
- `<vector>` - Dynamic array container

## Core Features

### 1. Match Flags Enumeration

```cpp
enum class [[gnu::packed]] MatchFlags : uint16_t {
    EMPTY = 0,

    // Basic numeric types
    U8B = 1 << 0,   // Unsigned 8-bit
    S8B = 1 << 1,   // Signed 8-bit
    U16B = 1 << 2,  // Unsigned 16-bit
    S16B = 1 << 3,  // Signed 16-bit
    U32B = 1 << 4,  // Unsigned 32-bit
    S32B = 1 << 5,  // Signed 32-bit
    U64B = 1 << 6,  // Unsigned 64-bit
    S64B = 1 << 7,  // Signed 64-bit

    // Floating point types
    F32B = 1 << 8,  // 32-bit float
    F64B = 1 << 9,  // 64-bit float

    // Composite types
    I8B = U8B | S8B,      // Any 8-bit integer
    I16B = U16B | S16B,   // Any 16-bit integer
    I32B = U32B | S32B,   // Any 32-bit integer
    I64B = U64B | S64B,   // Any 64-bit integer

    INTEGER = I8B | I16B | I32B | I64B,  // All integer types
    FLOAT = F32B | F64B,                  // All floating point types
    ALL = INTEGER | FLOAT,                // All supported types

    // Byte-based groupings
    B8 = I8B,           // 8-bit block
    B16 = I16B,         // 16-bit block
    B32 = I32B | F32B,  // 32-bit block
    B64 = I64B | F64B,  // 64-bit block

    MAX = 0xffffU  // Maximum flag value
};
```

### 2. Value Structure

The primary value container supporting multiple data types.

```cpp
struct [[gnu::packed]] Value {
    std::variant<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t,
                 uint64_t, float, double, std::array<uint8_t, sizeof(int64_t)>,
                 std::array<char, sizeof(int64_t)>> value;

    MatchFlags flags = MatchFlags::EMPTY;

    // Static utility methods
    constexpr static void zero(Value& val);
};
```

#### Data Types Supported

| C++ Type | MatchFlag | Description |
|----------|-----------|-------------|
| `int8_t` | `S8B` | Signed 8-bit integer |
| `uint8_t` | `U8B` | Unsigned 8-bit integer |
| `int16_t` | `S16B` | Signed 16-bit integer |
| `uint16_t` | `U16B` | Unsigned 16-bit integer |
| `int32_t` | `S32B` | Signed 32-bit integer |
| `uint32_t` | `U32B` | Unsigned 32-bit integer |
| `int64_t` | `S64B` | Signed 64-bit integer |
| `uint64_t` | `U64B` | Unsigned 64-bit integer |
| `float` | `F32B` | 32-bit floating point |
| `double` | `F64B` | 64-bit floating point |
| `std::array<uint8_t, 8>` | - | Raw byte array |
| `std::array<char, 8>` | - | Character array |

#### Static Methods

##### zero(Value& val)

Resets a Value to zero state with EMPTY flags.

```cpp
Value val = uint32_t{42};
Value::zero(val);  // val.value = int64_t{0}, val.flags = MatchFlags::EMPTY
```

### 3. Mem64 Structure

Memory-aligned 64-bit value container with type-safe access.

```cpp
struct [[gnu::packed]] Mem64 {
    std::variant<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t,
                 uint64_t, float, double, std::array<uint8_t, sizeof(int64_t)>,
                 std::array<char, sizeof(int64_t)>> mem64Value;

    // Type-safe access methods
    template <typename T>
    T get() const;

    template <typename Visitor>
    void visit(Visitor&& visitor) const;

    template <typename T>
    void set(const T& value);
};
```

#### Methods

##### get\<T\>()

Type-safe value retrieval with compile-time type checking.

```cpp
Mem64 mem64 = float{3.14f};
float value = mem64.get<float>();  // Returns 3.14f
// int value = mem64.get<int>();  // Would throw std::bad_variant_access
```

##### visit\<Visitor\>()

Generic visitor pattern for variant access.

```cpp
mem64.visit([](auto&& arg) {
    std::cout << "Value: " << arg << std::endl;
});
```

##### set\<T\>()

Type-safe value assignment with bit-casting for numeric types.

```cpp
Mem64 mem64;
mem64.set<double>(2.71828);
// mem64.mem64Value now contains double{2.71828}
```

### 4. Wildcard Enumeration

```cpp
enum class Wildcard : uint8_t {
    FIXED = 0xffU,    // Fixed value (no wildcard)
    WILDCARD = 0x00U  // Wildcard value (matches anything)
};
```

### 5. UserValue Structure

Comprehensive user input value representation with optional fields.

```cpp
struct [[gnu::packed]] UserValue {
    // Basic numeric values
    int8_t int8_value = 0;
    uint8_t uint8_value = 0;
    int16_t int16_value = 0;
    uint16_t uint16_value = 0;
    int32_t int32_value = 0;
    uint32_t uint32_value = 0;
    int64_t int64_value = 0;
    uint64_t uint64_value = 0;
    float float32_value = 0.0F;
    double float64_value = 0.0;

    // Optional complex types
    std::optional<std::vector<uint8_t>> bytearray_value;
    std::optional<Wildcard> wildcard_value;

    // String and flags
    std::string string_value;
    MatchFlags flags = MatchFlags::EMPTY;
};
```

## Usage Examples

### Basic Value Creation

```cpp
import value;

// Create values with different types
Value uint8_val = uint8_t{255};
uint8_val.flags = MatchFlags::U8B;

Value int32_val = int32_t{-42};
int32_val.flags = MatchFlags::S32B;

Value float_val = float{3.14f};
float_val.flags = MatchFlags::F32B;
```

### Working with Mem64

```cpp
Mem64 mem64;

// Store different types
mem64.set<int64_t>(INT64_MAX);
auto int64_value = mem64.get<int64_t>();

mem64.set<double>(M_PI);
auto double_value = mem64.get<double>();

// Visit the stored value
mem64.visit([](auto&& val) {
    using T = std::decay_t<decltype(val)>;
    if constexpr (std::is_same_v<T, double>) {
        std::cout << "Double value: " << val << std::endl;
    }
});
```

### User Value Input

```cpp
UserValue user_val;

// Set basic values
user_val.int32_value = 42;
user_val.float64_value = 1.23;
user_val.flags = MatchFlags::I32B | MatchFlags::F64B;

// Set string value
user_val.string_value = "test_string";

// Set byte array
user_val.bytearray_value = std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04};

// Set wildcard
user_val.wildcard_value = Wildcard::WILDCARD;
```

### Flag Operations

```cpp
// Check if flags include specific type
bool is_integer = (flags & MatchFlags::INTEGER) != MatchFlags::EMPTY;
bool is_float = (flags & MatchFlags::FLOAT) != MatchFlags::EMPTY;

// Check specific bit width
bool is_32bit = (flags & MatchFlags::B32) != MatchFlags::EMPTY;

// Combine flags
MatchFlags combined = MatchFlags::U8B | MatchFlags::U16B | MatchFlags::U32B;
```

### Byte Array Handling

```cpp
// Create value with byte array
std::array<uint8_t, 8> bytes{0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
Value byte_val = bytes;

// Access via visitor
byte_val.value.visit([](auto&& arg) {
    if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, 
                                std::array<uint8_t, 8>>) {
        for (uint8_t b : arg) {
            std::cout << std::hex << static_cast<int>(b) << " ";
        }
    }
});
```

## Memory Layout and Packing

### Structure Packing

All structures use `[[gnu::packed]]` attribute to minimize memory usage:

- **Value**: ~72 bytes (variant + flags + padding)
- **Mem64**: ~72 bytes (variant only)
- **UserValue**: ~200+ bytes (including string and vectors)

### Alignment

Structures are packed to minimize memory usage, which may impact performance on some architectures.

## Type Safety and Error Handling

### Runtime Type Checking

```cpp
try {
    Mem64 mem64 = int32_t{42};
    double val = mem64.get<double>();  // Throws std::bad_variant_access
} catch (const std::bad_variant_access& e) {
    std::cerr << "Type mismatch: " << e.what() << std::endl;
}
```

### Compile-time Type Checking

```cpp
// Static assertions ensure type safety
static_assert(std::is_trivially_copyable_v<int64_t>);
static_assert(sizeof(std::array<uint8_t, 8>) == 8);
```

## Integration Examples

### With Target Memory Module

```cpp
import value;
import targetmem;

void create_match_entry(void* addr, uint8_t byte, Value::value_type value) {
    OldValueAndMatchInfo info;
    info.old_value = byte;
    
    // Determine match flags based on value type
    info.match_info = std::visit([](auto&& arg) -> MatchFlags {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, uint8_t>) return MatchFlags::U8B;
        else if constexpr (std::is_same_v<T, int8_t>) return MatchFlags::S8B;
        else if constexpr (std::is_same_v<T, uint16_t>) return MatchFlags::U16B;
        // ... handle other types
    }, value);
}
```

### With Endianness Module

```cpp
import value;
import endianness;

void handle_endianness(Value& val, bool reverse_endianness) {
    if (reverse_endianness) {
        endianness::fix_endianness(val, true);
    }
}
```

## Testing

```cpp
void test_value_module() {
    // Test Value creation
    Value val = uint32_t{42};
    assert(std::get<uint32_t>(val.value) == 42);
    
    // Test Mem64
    Mem64 mem64;
    mem64.set<float>(3.14f);
    assert(mem64.get<float>() == 3.14f);
    
    // Test flags
    MatchFlags flags = MatchFlags::U8B | MatchFlags::U16B;
    assert((flags & MatchFlags::U8B) != MatchFlags::EMPTY);
    
    // Test UserValue
    UserValue user_val;
    user_val.int32_value = 123;
    assert(user_val.int32_value == 123);
    
    std::cout << "All tests passed!\n";
}
```

## Performance Considerations

### Memory Usage

- **Value**: Minimal overhead with variant storage
- **Mem64**: Fixed 64-bit storage
- **UserValue**: Larger due to optional containers and string

### Runtime Performance

- **Variant access**: O(1) via std::visit or std::get
- **Type checking**: Compile-time with std::get_if
- **Memory alignment**: Packed structures may have alignment overhead

## Best Practices

1. **Use appropriate types**: Choose specific numeric types over generic ones
2. **Initialize flags**: Always set appropriate MatchFlags
3. **Handle type mismatches**: Use try-catch for Mem64::get\<T\>()
4. **Consider alignment**: Packed structures may impact performance
5. **Use visitors**: Prefer std::visit for type-safe variant access

## See Also

- [Target Memory Module](target_mem.md) - Uses Value and MatchFlags
- [Endianness Module](endianness.md) - Handles byte order for values
- [Show Message Module](show_message.md) - For value reporting and debugging

## Future Enhancements

- Support for 128-bit types
- SIMD type support
- Custom allocator support
- Serialization/deserialization
- JSON conversion utilities
- Memory-mapped file support
- Zero-copy operations
- Type reflection capabilities
- Performance profiling utilities
- Cross-platform endianness detection helpers

## Platform Compatibility

- **Linux**: Full support
- **macOS**: Full support
- **Windows**: Full support
- **32-bit systems**: Limited 64-bit type support
- **Big-endian**: Full support with appropriate endianness handling

## Migration Guide

### From C-style unions

```cpp
// Old C-style union
union ValueUnion {
    uint8_t u8;
    uint16_t u16;
    uint32_t u32;
    float f32;
};

// New C++23 variant
Value val = uint32_t{42};  // Type-safe and extensible
```

### From raw pointers

```cpp
// Old raw pointer approach
void* ptr = malloc(sizeof(int32_t));
*(int32_t*)ptr = 42;

// New type-safe approach
Mem64 mem64;
mem64.set<int32_t>(42);  // Safe and automatic cleanup
