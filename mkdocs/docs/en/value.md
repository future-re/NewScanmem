# Value Module Documentation

## Overview

The `value` module provides comprehensive value type definitions and match flags for the NewScanmem project. It defines various data type representations, memory layouts, and utility structures for handling different numeric types, byte arrays, and wildcard patterns used in memory scanning operations.

## Module Structure

```cpp
export module value;
```

## Dependencies

- `<cstdint>` - Fixed-width integer types
- `<cstring>` - Byte copy
- `<optional>` - Optional type support
- `<span>` - Byte views
- `<string>` - String operations
- `<type_traits>` - Type utilities
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

### 2. Value Structure (byte-centric)

Value stores historical (old) values as raw bytes. Type/width semantics are carried by `flags`.

```cpp
struct [[gnu::packed]] Value {
    std::vector<uint8_t> bytes;     // Snapshot bytes
    MatchFlags flags = MatchFlags::EMPTY; // Type/width flag (required for numeric)

    constexpr static void zero(Value& val);

    // Views and setters
    std::span<const uint8_t> view() const noexcept;
    void setBytes(const uint8_t* data, std::size_t len);
    void setBytes(const std::vector<uint8_t>& val);
    void setBytesWithFlag(const uint8_t* data, std::size_t len, MatchFlags f);
    void setBytesWithFlag(const std::vector<uint8_t>& val, MatchFlags f);

    template <typename T> void setScalar(const T& v);
    template <typename T> void setScalarWithFlag(const T& v, MatchFlags f);
    template <typename T> void setScalarTyped(const T& v); // auto-set correct flag
};
```

Notes:
- Numeric comparisons are strict: old value is decoded only if `flags` matches the requested type and the byte width is sufficient.
- Byte array/string comparisons do not rely on `flags`.

### 3. Mem64 Structure (current bytes buffer)

Mem64 represents the current bytes read at a memory location.

```cpp
struct [[gnu::packed]] Mem64 {
    std::vector<uint8_t> buffer;

    template <typename T> T get() const;              // memcpy decode
    std::span<const uint8_t> bytes() const noexcept;  // read-only view
    void setBytes(const uint8_t* data, std::size_t len);
    void setBytes(const std::vector<uint8_t>& data);
    void setString(const std::string& s);
    template <typename T> void setScalar(const T& v);
};
```

#### Methods

- `get<T>()`: Decode the first `sizeof(T)` bytes using `memcpy` (throws if insufficient bytes).
- `bytes()`: Read-only span view over underlying bytes.
- `setBytes(...)`, `setString(...)`, `setScalar<T>(...)`: Write helpers.

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
    std::optional<std::vector<uint8_t>> byteMask; // 0xFF=fixed, 0x00=wildcard
    std::optional<Wildcard> wildcard_value;

    // String and flags
    std::string string_value;
    MatchFlags flags = MatchFlags::EMPTY;
};
```

## Usage Examples

### Basic Value Creation (strict numeric + free-form bytes)

```cpp
import value;

// Create values with different types
Value uint8_val; uint8_val.setScalarTyped<uint8_t>(255);
Value int32_val; int32_val.setScalarTyped<int32_t>(-42);
Value float_val; float_val.setScalarTyped<float>(3.14f);
```

### Working with Mem64

```cpp
Mem64 mem64;
mem64.setScalar<int64_t>(INT64_MAX);
auto int64_value = mem64.get<int64_t>();

mem64.setScalar<double>(M_PI);
auto double_value = mem64.get<double>();
```

### User Value Input

```cpp
UserValue user_val;

// Set numeric values if needed
user_val.int32_value = 42;
user_val.float64_value = 1.23;

// Set string value
user_val.string_value = "test_string";

// Set byte array
user_val.bytearray_value = std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04};

// Optional wildcard intent (used with masked matching at higher level)
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
Value byte_val;
std::vector<uint8_t> bytes{0x01, 0x02, 0x03, 0x04};
byte_val.setBytes(bytes);

// Iterate bytes via view
for (auto b : byte_val.view()) {
    std::cout << std::hex << static_cast<int>(b) << " ";
}
```

## Memory Layout and Packing

### Structure Packing

All structures use `[[gnu::packed]]` attribute to minimize memory usage:

- **Value**: bytes vector + flag
- **Mem64**: bytes vector buffer
- **UserValue**: ~200+ bytes (including string and vectors)

### Alignment

Structures are packed to minimize memory usage, which may impact performance on some architectures.

## Type Safety and Error Handling

### Runtime Type Checking

```cpp
Mem64 mem64;
mem64.setScalar<int32_t>(42);
// Decoding to a mismatched type or with insufficient bytes throws
double val = mem64.get<double>();
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

void create_match_entry(void* addr, uint8_t byte, const Value& value) {
    OldValueAndMatchInfo info;
    info.old_value = byte;
    // Determine flags from context (example: 8-bit unsigned)
    info.match_info = MatchFlags::U8B;
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
    Value val; val.setScalarTyped<uint32_t>(42);
    
    // Test Mem64
    Mem64 mem64;
    mem64.setScalar<float>(3.14f);
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

- **Value**: Byte vector storage + strict flags
- **Mem64**: Byte buffer with memcpy decode
- **UserValue**: Larger due to optional containers and string

### Runtime Performance

- **Variant access**: O(1) via std::visit or std::get
- **Type checking**: Compile-time with std::get_if
- **Memory alignment**: Packed structures may have alignment overhead

## Best Practices

1. **Use appropriate types**: Choose specific numeric types over generic ones
2. **Initialize flags**: Always set appropriate MatchFlags
3. **Handle type mismatches**: Guard reads and check sizes for `get<T>()`
4. **Consider alignment**: Packed structures may impact performance

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
