# Target Memory Module Documentation

## Overview

The `targetmem` module provides memory matching and analysis structures for the NewScanmem project. It includes classes for managing memory matches, storing old values, and performing memory region operations with support for efficient searching and manipulation.

## Module Structure

```cpp
export module targetmem;
```

## Dependencies

- `<algorithm>` - Standard algorithms
- `<cassert>` - Assert macros
- `<cctype>` - Character classification
- `<cstddef>` - Size type definitions
- `<cstdint>` - Fixed-width integer types
- `<cstdio>` - C standard I/O
- `<cstdlib>` - C standard library
- `<cstring>` - C string operations
- `<optional>` - Optional type support
- `<sstream>` - String stream operations
- `<string>` - String operations
- `<vector>` - Dynamic array container
- `value` module - Value type definitions
- `show_message` module - Message printing system

## Core Features

### 1. Memory Match Flags

Uses `MatchFlags` from the `value` module to indicate the type and status of memory matches.

### 2. OldValueAndMatchInfo Structure

```cpp
struct OldValueAndMatchInfo {
    uint8_t old_value;      // The original byte value
    MatchFlags match_info;  // Match type and status flags
};
```

### 3. MatchesAndOldValuesSwath Class

Represents a contiguous memory region with match information.

```cpp
class MatchesAndOldValuesSwath {
public:
    void* firstByteInChild = nullptr;                    // Starting address
    std::vector<OldValueAndMatchInfo> data;              // Match data

    // Constructors
    MatchesAndOldValuesSwath() = default;

    // Element management
    void addElement(void* addr, uint8_t byte, MatchFlags matchFlags);

    // String representation utilities
    std::string toPrintableString(size_t idx, size_t len) const;
    std::string toByteArrayText(size_t idx, size_t len) const;
};
```

#### Methods

##### addElement(void* addr, uint8_t byte, MatchFlags matchFlags)

Adds a new memory match to the swath.

**Parameters:**

- `addr`: Memory address of the match
- `byte`: The byte value at this address
- `matchFlags`: Type of match and flags

##### toPrintableString(size_t idx, size_t len)

Converts memory bytes to a printable ASCII string.

**Parameters:**

- `idx`: Starting index in the data vector
- `len`: Number of bytes to convert

**Returns:** Printable string with non-printable characters as '.'

##### toByteArrayText(size_t idx, size_t len)

Converts memory bytes to hexadecimal text representation.

**Parameters:**

- `idx`: Starting index in the data vector
- `len`: Number of bytes to convert

**Returns:** Space-separated hexadecimal values

### 4. MatchesAndOldValuesArray Class

Manages multiple swaths and provides search operations.

```cpp
class MatchesAndOldValuesArray {
public:
    size_t maxNeededBytes;                          // Maximum memory needed
    std::vector<MatchesAndOldValuesSwath> swaths;   // Collection of swaths

    // Constructor
    MatchesAndOldValuesArray(size_t maxBytes);

    // Swath management
    void addSwath(const MatchesAndOldValuesSwath& swath);

    // Search operations
    std::optional<std::pair<MatchesAndOldValuesSwath*, size_t>> nthMatch(size_t n);

    // Memory range operations
    void deleteInAddressRange(void* start, void* end, unsigned long& numMatches);
};
```

#### MatchesAndOldValuesArray Methods

##### Constructor(size_t maxBytes)

Initializes with maximum memory requirement.

**Parameters:**

- `maxBytes`: Maximum bytes needed for all matches

##### addSwath(const MatchesAndOldValuesSwath& swath)

Adds a new swath to the collection.

##### nthMatch(size_t n)

Finds the nth valid match across all swaths.

**Parameters:**

- `n`: Match index to find (0-based)

**Returns:** Optional pair of (swath pointer, index within swath) or nullopt if not found

##### deleteInAddressRange(void*start, void* end, unsigned long& numMatches)

Removes all matches within a specified address range.

**Parameters:**

- `start`: Starting address of range (inclusive)
- `end`: Ending address of range (exclusive)
- `numMatches`: Output parameter counting removed matches

## Usage Examples

### Creating and Populating Swaths

```cpp
import targetmem;

MatchesAndOldValuesSwath swath;

// Add memory matches
void* base_addr = reinterpret_cast<void*>(0x1000);
swath.addElement(base_addr, 0x41, MatchFlags::U8B);        // 'A' character
swath.addElement(static_cast<char*>(base_addr) + 1, 0x42, MatchFlags::U8B); // 'B' character
swath.addElement(static_cast<char*>(base_addr) + 2, 0x00, MatchFlags::EMPTY); // No match
```

### String Representations

```cpp
// Get printable representation
std::string printable = swath.toPrintableString(0, 3);
// Result: "AB."

// Get hex representation
std::string hex = swath.toByteArrayText(0, 3);
// Result: "41 42 0"
```

### Managing Multiple Swaths

```cpp
MatchesAndOldValuesArray array(1024); // Max 1KB of matches

// Add swaths
array.addSwath(swath1);
array.addSwath(swath2);

// Find specific matches
auto match = array.nthMatch(5);
if (match) {
    auto [swath_ptr, index] = *match;
    // Process the 6th match (0-based index 5)
}
```

### Address Range Operations

```cpp
// Remove matches in a specific address range
unsigned long removed_count = 0;
void* start_range = reinterpret_cast<void*>(0x1000);
void* end_range = reinterpret_cast<void*>(0x2000);

array.deleteInAddressRange(start_range, end_range, removed_count);
std::cout << "Removed " << removed_count << " matches\n";
```

## Memory Layout and Addressing

### Address Calculation

The module uses pointer arithmetic to calculate actual memory addresses:

```cpp
// Calculate actual address from swath data
void* actual_address = static_cast<char*>(swath.firstByteInChild) + index;
```

### Swath Organization

Each swath represents a contiguous memory region with:

- Fixed starting address (`firstByteInChild`)
- Sequential byte values (`data` vector)
- Match flags for each byte

## Performance Considerations

### Memory Usage

- **Swath overhead**: Each swath has minimal overhead
- **Data storage**: Stores one byte + flags per address
- **Vector growth**: Uses std::vector with amortized growth

### Search Performance

- **nthMatch**: O(n) traversal across all swaths
- **Range deletion**: O(n) with efficient std::remove_if
- **Memory locality**: Good cache performance due to vector storage

## Error Handling

### Bounds Checking

All methods include bounds checking:

- `toPrintableString` and `toByteArrayText` use `std::min` to prevent overflow
- `nthMatch` returns `std::nullopt` for invalid indices

### Null Pointer Safety

- `firstByteInChild` is initialized to nullptr
- Methods verify vector sizes before access

## Advanced Usage

### Complex Memory Analysis

```cpp
void analyze_memory_region(void* start, size_t size) {
    MatchesAndOldValuesArray matches(size);
    MatchesAndOldValuesSwath swath;
    
    swath.firstByteInChild = start;
    
    // Analyze each byte
    for (size_t i = 0; i < size; ++i) {
        void* addr = static_cast<char*>(start) + i;
        uint8_t value = read_memory_byte(addr); // Implementation specific
        
        MatchFlags flags = analyze_value(value); // Determine match type
        swath.addElement(addr, value, flags);
    }
    
    matches.addSwath(swath);
    
    // Process results
    auto first_match = matches.nthMatch(0);
    if (first_match) {
        // Found at least one match
    }
}
```

### Memory Pattern Matching

```cpp
void find_string_patterns(const MatchesAndOldValuesArray& matches) {
    for (const auto& swath : matches.swaths) {
        for (size_t i = 0; i < swath.data.size(); ++i) {
            if (swath.data[i].match_info != MatchFlags::EMPTY) {
                // Check for ASCII string patterns
                std::string chars = swath.toPrintableString(i, 16);
                if (is_printable_string(chars)) {
                    void* addr = static_cast<char*>(swath.firstByteInChild) + i;
                    std::cout << "String at " << addr << ": " << chars << "\n";
                }
            }
        }
    }
}
```

## Integration with Other Modules

### With Show Message Module

```cpp
import targetmem;
import show_message;

void report_matches(const MatchesAndOldValuesArray& matches) {
    MessagePrinter printer;
    
    size_t total_matches = 0;
    for (const auto& swath : matches.swaths) {
        for (const auto& [value, flags] : swath.data) {
            if (flags != MatchFlags::EMPTY) {
                total_matches++;
            }
        }
    }
    
    printer.info("Found {} total matches", total_matches);
}
```

### With Process Checker

```cpp
void scan_process_memory(pid_t pid) {
    if (ProcessChecker::is_process_dead(pid)) {
        return;
    }
    
    // Create memory scanner
    MatchesAndOldValuesArray matches(get_process_memory_size(pid));
    
    // Perform scan...
}
```

## Testing

```cpp
void test_target_memory() {
    // Test swath creation
    MatchesAndOldValuesSwath swath;
    swath.addElement(nullptr, 0x41, MatchFlags::U8B);
    
    assert(swath.data.size() == 1);
    assert(swath.data[0].old_value == 0x41);
    assert(swath.data[0].match_info == MatchFlags::U8B);
    
    // Test string representations
    std::string printable = swath.toPrintableString(0, 1);
    assert(printable == "A");
    
    std::string hex = swath.toByteArrayText(0, 1);
    assert(hex == "41");
    
    // Test array operations
    MatchesAndOldValuesArray array(1024);
    array.addSwath(swath);
    
    auto match = array.nthMatch(0);
    assert(match.has_value());
    
    std::cout << "All tests passed!\n";
}
```

## See Also

- [Value Module](value.md) - For MatchFlags definitions
- [Process Checker Module](process_checker.md) - For process state checking
- [Show Message Module](show_message.md) - For logging and reporting

## Future Enhancements

- Support for larger memory regions
- Compression for sparse match data
- Parallel processing support
- Memory scanning optimizations
- Integration with memory mapping APIs
- Support for different memory types (heap, stack, code)
- Pattern matching algorithms
- Memory write operations
- Snapshot and comparison features

## Thread Safety

The classes are not thread-safe by default. For concurrent access:

1. Use external synchronization
2. Consider immutable copies for read-only access
3. Use separate instances for different threads

## Memory Management

- Automatic memory management via std::vector
- RAII principles for exception safety
- Efficient move semantics for large datasets
- Minimal heap allocations during operations
