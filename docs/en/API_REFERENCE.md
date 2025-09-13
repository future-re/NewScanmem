# API Reference

## Module Index

- [endianness](#endianness)
- [maps](#maps)
- [process_checker](#process_checker)
- [sets](#sets)
- [show_message](#show_message)
- [targetmem](#targetmem)
- [value](#value)

---

## maps

### Enum: `region_type`

```cpp
enum class region_type : uint8_t {
    misc,   // Miscellaneous memory regions
    exe,    // Executable binary regions
    code,   // Code segments (shared libraries, etc.)
    heap,   // Heap memory regions
    stack   // Stack memory regions
};

constexpr std::array<std::string_view, 5> region_type_names;
```

### Enum: `region_scan_level`

```cpp
enum class region_scan_level : uint8_t {
    all,                       // All readable regions
    all_rw,                    // All readable/writable regions
    heap_stack_executable,     // Heap, stack, and executable regions
    heap_stack_executable_bss  // Above plus BSS segments
};
```

### Struct: `region_flags`

```cpp
struct region_flags {
    bool read : 1;    // Read permission
    bool write : 1;   // Write permission
    bool exec : 1;    // Execute permission
    bool shared : 1;  // Shared mapping
    bool private_ : 1; // Private mapping
};
```

### Struct: `region`

```cpp
struct region {
    void* start;           // Starting address
    std::size_t size;      // Region size in bytes
    region_type type;      // Region classification
    region_flags flags;    // Permission flags
    void* load_addr;       // Load address for ELF files
    std::string filename;  // Associated file path
    std::size_t id;        // Unique identifier

    [[nodiscard]] bool is_readable() const noexcept;
    [[nodiscard]] bool is_writable() const noexcept;
    [[nodiscard]] bool is_executable() const noexcept;
    [[nodiscard]] bool is_shared() const noexcept;
    [[nodiscard]] bool is_private() const noexcept;
    [[nodiscard]] std::pair<void*, std::size_t> as_span() const noexcept;
    [[nodiscard]] bool contains(void* address) const noexcept;
};
```

### Struct: `maps_reader::error`

```cpp
struct error {
    std::string message;   // Human-readable error description
    std::error_code code;  // System error code
};
```

### Class: `maps_reader`

#### Static Methods

```cpp
[[nodiscard]] static std::expected<std::vector<region>, error> 
read_process_maps(pid_t pid, region_scan_level level = region_scan_level::all);
```

### Convenience Functions

```cpp
[[nodiscard]] std::expected<std::vector<region>, maps_reader::error> 
read_process_maps(pid_t pid, region_scan_level level = region_scan_level::all);
```

#### Usage Examples

```cpp
import maps;

// Basic usage
auto regions = maps::read_process_maps(1234);

// Filtered scanning
auto heap_regions = maps::read_process_maps(
    pid, 
    maps::region_scan_level::heap_stack_executable
);

// Error handling
if (!regions) {
    std::cerr << "Error: " << regions.error().message << "\n";
}
```

---

## endianness

### Namespace: `endianness`

#### Functions

```cpp
// Endianness detection
constexpr bool is_big_endian() noexcept;
constexpr bool is_little_endian() noexcept;

// Byte swapping
constexpr uint8_t swap_bytes(uint8_t value) noexcept;
constexpr uint16_t swap_bytes(uint16_t value) noexcept;
constexpr uint32_t swap_bytes(uint32_t value) noexcept;
constexpr uint64_t swap_bytes(uint64_t value) noexcept;

template<typename T>
constexpr T swap_bytes_integral(T value) noexcept;

void swap_bytes_inplace(void* data, size_t size);

// Endianness correction
void fix_endianness(Value& value, bool reverse_endianness) noexcept;

// Network byte order
template<SwappableIntegral T>
constexpr T host_to_network(T value) noexcept;

template<SwappableIntegral T>
constexpr T network_to_host(T value) noexcept;

// Little-endian conversion
template<SwappableIntegral T>
constexpr T host_to_little_endian(T value) noexcept;

template<SwappableIntegral T>
constexpr T little_endian_to_host(T value) noexcept;
```

#### Concept

```cpp
template<typename T>
concept SwappableIntegral = std::integral<T> && 
    (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);
```

---

## process_checker

### Enum: `ProcessState`

```cpp
enum class ProcessState { RUNNING, ERROR, DEAD, ZOMBIE };
```

### Class: `ProcessChecker`

#### ProcessChecker Static Methods

```cpp
static ProcessState check_process(pid_t pid);
static bool is_process_dead(pid_t pid);
```

---

## sets

### Struct: `Set`

```cpp
struct Set {
    std::vector<size_t> buf;
    
    size_t size() const;
    void clear();
    static int cmp(const size_t& i1, const size_t& i2);
};
```

#### Set Functions

```cpp
bool parse_uintset(std::string_view lptr, Set& set, size_t maxSZ);
```

#### Deprecated

```cpp
[[deprecated]] constexpr auto inc_arr_sz = 
    [](size_t** valarr, size_t* arr_maxsz, size_t maxsz) -> bool;
```

---

## show_message

### Enum: `MessageType`

```cpp
enum class MessageType : uint8_t { INFO, WARN, ERROR, DEBUG, USER };
```

### Struct: `MessageContext`

```cpp
struct MessageContext {
    bool debugMode = false;
    bool backendMode = false;
};
```

### Class: `MessagePrinter`

#### Constructor

```cpp
MessagePrinter(MessageContext ctx = {});
```

#### Methods

```cpp
template<typename... Args>
void print(MessageType type, std::string_view fmt, Args&&... args) const;

template<typename... Args>
void info(std::string_view fmt, Args&&... args) const;

template<typename... Args>
void warn(std::string_view fmt, Args&&... args) const;

template<typename... Args>
void error(std::string_view fmt, Args&&... args) const;

template<typename... Args>
void debug(std::string_view fmt, Args&&... args) const;

template<typename... Args>
void user(std::string_view fmt, Args&&... args) const;

[[nodiscard]] const MessageContext& conext() const;
```

---

## targetmem

### Struct: `OldValueAndMatchInfo`

```cpp
struct OldValueAndMatchInfo {
    uint8_t old_value;
    MatchFlags match_info;
};
```

### Class: `MatchesAndOldValuesSwath`

```cpp
class MatchesAndOldValuesSwath {
public:
    void* firstByteInChild = nullptr;
    std::vector<OldValueAndMatchInfo> data;

    MatchesAndOldValuesSwath() = default;

    void addElement(void* addr, uint8_t byte, MatchFlags matchFlags);
    std::string toPrintableString(size_t idx, size_t len) const;
    std::string toByteArrayText(size_t idx, size_t len) const;
};
```

### Class: `MatchesAndOldValuesArray`

```cpp
class MatchesAndOldValuesArray {
public:
    size_t maxNeededBytes;
    std::vector<MatchesAndOldValuesSwath> swaths;

    MatchesAndOldValuesArray(size_t maxBytes);

    void addSwath(const MatchesAndOldValuesSwath& swath);
    std::optional<std::pair<MatchesAndOldValuesSwath*, size_t>> nthMatch(size_t n);
    void deleteInAddressRange(void* start, void* end, unsigned long& numMatches);
};
```

---

## value

### Enum: `MatchFlags`

```cpp
enum class [[gnu::packed]] MatchFlags : uint16_t {
    EMPTY = 0,
    U8B = 1 << 0, S8B = 1 << 1,
    U16B = 1 << 2, S16B = 1 << 3,
    U32B = 1 << 4, S32B = 1 << 5,
    U64B = 1 << 6, S64B = 1 << 7,
    F32B = 1 << 8, F64B = 1 << 9,
    I8B = U8B | S8B, I16B = U16B | S16B,
    I32B = U32B | S32B, I64B = U64B | S64B,
    INTEGER = I8B | I16B | I32B | I64B,
    FLOAT = F32B | F64B,
    ALL = INTEGER | FLOAT,
    B8 = I8B, B16 = I16B,
    B32 = I32B | F32B, B64 = I64B | F64B,
    MAX = 0xffffU
};
```

### Enum: `Wildcard`

```cpp
enum class Wildcard : uint8_t { FIXED = 0xffU, WILDCARD = 0x00U };
```

### Struct: `Value`

```cpp
struct [[gnu::packed]] Value {
    std::vector<uint8_t> bytes;     // Snapshot bytes
    MatchFlags flags = MatchFlags::EMPTY; // Type/width flag

    constexpr static void zero(Value& val);

    std::span<const uint8_t> view() const noexcept;
    void setBytes(const uint8_t* data, std::size_t len);
    void setBytes(const std::vector<uint8_t>& val);
    void setBytesWithFlag(const uint8_t* data, std::size_t len, MatchFlags f);
    void setBytesWithFlag(const std::vector<uint8_t>& val, MatchFlags f);
    template <typename T> void setScalar(const T& v);
    template <typename T> void setScalarWithFlag(const T& v, MatchFlags f);
    template <typename T> void setScalarTyped(const T& v);
};
```

Strict numeric decoding: the requested type must match `flags` and width must be sufficient.

### Struct: `Mem64`

```cpp
struct [[gnu::packed]] Mem64 {
    std::vector<uint8_t> buffer;    // Current bytes

    template <typename T> T get() const;              // memcpy-based decode
    std::span<const uint8_t> bytes() const noexcept;  // read-only view
    void setBytes(const uint8_t* data, std::size_t len);
    void setBytes(const std::vector<uint8_t>& data);
    void setString(const std::string& s);
    template <typename T> void setScalar(const T& v);
};
```

### Struct: `UserValue`

```cpp
struct [[gnu::packed]] UserValue {
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
    std::optional<std::vector<uint8_t>> bytearray_value;
    std::optional<std::vector<uint8_t>> byteMask; // 0xFF=fixed, 0x00=wildcard
    std::optional<Wildcard> wildcard_value;
    std::string string_value;
    MatchFlags flags = MatchFlags::EMPTY;
};
```

## Usage Patterns

### Common Patterns

#### 1. Process Checking

```cpp
auto state = ProcessChecker::check_process(pid);
if (state == ProcessState::RUNNING) { /* ... */ }
```

#### 2. Set Parsing

```cpp
Set mySet;
if (parse_uintset("1,2,3,10..15", mySet, 100)) {
    // Use parsed set
}
```

#### 3. Message Printing

```cpp
MessagePrinter printer{.debugMode = true};
printer.info("Process {} is running", pid);
printer.error("Failed to read memory at 0x{:08x}", address);
```

#### 4. Memory Analysis

```cpp
MatchesAndOldValuesArray matches(4096);
MatchesAndOldValuesSwath swath;
swath.addElement(addr, byte, MatchFlags::U32B);
matches.addSwath(swath);
```

#### 5. Value Handling

```cpp
Value val = uint32_t{42};
val.flags = MatchFlags::U32B;

Mem64 mem64;
mem64.set<double>(3.14159);
auto value = mem64.get<double>();
```

## Error Handling

### Return Values

- `parse_uintset`: Returns `bool` (true/false)
- `nthMatch`: Returns `std::optional`
- `check_process`: Returns `ProcessState`

### Exceptions

- `Mem64::get<T>()`: Throws `std::bad_variant_access` on type mismatch
- Format strings: Throw appropriate exceptions on format errors
