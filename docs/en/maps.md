# Maps Module Documentation

## Overview

The Maps module provides functionality to read and parse Linux `/proc/[pid]/maps` files, extracting memory region information for process analysis. This is a modern C++20 implementation that replaces the legacy C code with type-safe, RAII-compliant interfaces.

## Module Structure

```cpp
import maps;
```

## Core Components

### 1. Region Types

#### `RegionType` Enum

Represents the classification of memory regions:

```cpp
enum class RegionType : uint8_t {
    MISC,   // Miscellaneous memory regions
    EXE,    // Executable binary regions
    CODE,   // Code segments (shared libraries, etc.)
    HEAP,   // Heap memory regions
    STACK   // Stack memory regions
};

constexpr std::array<std::string_view, 5> REGION_TYPE_NAMES = {
    "misc", "exe", "code", "heap", "stack"
};
```

### 2. Scan Levels

#### `RegionScanLevel` Enum

Controls which memory regions are included in scanning:

```cpp
enum class RegionScanLevel : uint8_t {
    ALL,                       // All readable regions
    ALL_RW,                    // All readable/writable regions
    HEAP_STACK_EXECUTABLE,     // Heap, stack, and executable regions
    HEAP_STACK_EXECUTABLE_BSS  // Above plus BSS segments
};
```

### 3. Region Metadata

#### `RegionFlags` Structure

Contains permission and state flags for memory regions:

```cpp
struct RegionFlags {
    bool read : 1;    // Read permission
    bool write : 1;   // Write permission
    bool exec : 1;    // Execute permission
    bool shared : 1;  // Shared mapping
    bool private_ : 1; // Private mapping
};
```

#### `Region` Structure

Complete information about a memory region:

```cpp
struct Region {
    void* start;           // Starting address
    std::size_t size;      // Region size in bytes
    RegionType type;       // Region classification
    RegionFlags flags;     // Permission flags
    void* loadAddr;        // Load address for ELF files
    std::string filename;  // Associated file path
    std::size_t id;        // Unique identifier
    
    // Helper methods
    [[nodiscard]] bool isReadable() const noexcept;
    [[nodiscard]] bool isWritable() const noexcept;
    [[nodiscard]] bool isExecutable() const noexcept;
    [[nodiscard]] bool isShared() const noexcept;
    [[nodiscard]] bool isPrivate() const noexcept;
    
    [[nodiscard]] std::pair<void*, std::size_t> asSpan() const noexcept;
    [[nodiscard]] bool contains(void* address) const noexcept;
};
```

## Usage Examples

### Basic Usage

```cpp
import maps;

// Read all memory regions from process
auto result = maps::readProcessMaps(1234);
if (result) {
    for (const auto& region : *result) {
        std::cout << std::format("Region: {}-{} ({})\n", 
                               region.start, 
                               static_cast<char*>(region.start) + region.size,
                               REGION_TYPE_NAMES[static_cast<size_t>(region.type)]);
    }
}
```

### Filtered Scanning

```cpp
// Only scan heap and stack regions
auto regions = maps::readProcessMaps(
    pid, 
    maps::RegionScanLevel::HEAP_STACK_EXECUTABLE
);

if (regions) {
    for (const auto& region : *regions) {
        if (region.type == maps::RegionType::HEAP) {
            std::cout << "Heap region found: " << region.filename << "\n";
        }
    }
}
```

### Error Handling

```cpp
auto result = maps::readProcessMaps(pid);
if (!result) {
    std::cerr << "Error: " << result.error().message << "\n";
    return;
}
```

## Class: `MapsReader`

### Static Methods

#### `readProcessMaps`

Reads memory regions from a process:

```cpp
[[nodiscard]] static std::expected<std::vector<Region>, Error> 
readProcessMaps(pid_t pid, RegionScanLevel level = RegionScanLevel::ALL);
```

**Parameters:**

- `pid`: Target process ID
- `level`: Scan level filter (default: `all`)

**Returns:**

- `std::expected` containing vector of regions or error information

**Error Handling:**

- Returns `std::error_code` with appropriate error messages
- Common errors: file not found, permission denied, invalid format

## Error Handling Module

### `MapsReader::Error` Structure

```cpp
struct Error {
    std::string message;   // Human-readable error description
    std::error_code code;  // System error code
};
```

### Common Error Scenarios

1. **Process doesn't exist**: `no_such_file_or_directory`
2. **Permission denied**: `permission_denied`
3. **Invalid format**: `invalid_argument`

## Advanced Features

### Region Analysis

```cpp
// Check if address is within any region
auto regions = maps::readProcessMaps(pid);
void* address = /* some address */;

for (const auto& region : *regions) {
    if (region.contains(address)) {
        std::cout << "Address found in: " << region.filename << "\n";
        break;
    }
}
```

### Permission Checking

```cpp
// Find writable executable regions (potential shellcode targets)
for (const auto& region : *regions) {
    if (region.is_writable() && region.is_executable()) {
        std::cout << "WX region: " << region.filename << "\n";
    }
}
```

## Performance Notes

- **Memory Efficient**: Uses `std::string` with SSO for small filenames
- **Zero-Copy**: Direct string extraction from map lines
- **Early Filtering**: Regions filtered during parsing based on scan level
- **RAII**: Automatic resource cleanup via `std::ifstream`

## Thread Safety

- **Thread-Safe**: All methods are thread-safe for concurrent access
- **No Shared State**: Each call creates independent state
- **Immutable Results**: Returned vectors contain immutable data

## Platform Compatibility

- **Linux Only**: Requires `/proc/[pid]/maps` filesystem
- **C++23 Required**: Uses `std::expected` and other C++23 features
- **64-bit Support**: Handles both 32-bit and 64-bit address spaces

## Migration from Legacy C Code

### Legacy C → Modern C++

| Legacy C | Modern C++ |
|----------|------------|
| `region_t*` | `maps::Region` |
| `list_t` | `std::vector<Region>` |
| `bool return` | `std::expected` |
| `char*` filename | `std::string` |
| Manual memory management | RAII |
| Error codes | Exception-safe error handling |

## Examples

### Complete Working Example

```cpp
#include <iostream>
import maps;

int main() {
    pid_t target_pid = 1234; // Replace with actual PID
    
    auto regions = maps::readProcessMaps(target_pid);
    if (!regions) {
        std::cerr << "Failed to read maps: " << regions.error().message << "\n";
        return 1;
    }
    
    std::cout << "Found " << regions->size() << " memory regions:\n";
    
    for (const auto& region : *regions) {
        std::cout << std::format(
            "0x{:x}-0x{:x} {} {} {}\n",
            reinterpret_cast<uintptr_t>(region.start),
            reinterpret_cast<uintptr_t>(region.start) + region.size,
            region.is_readable() ? 'r' : '-',
            region.is_writable() ? 'w' : '-',
            region.is_executable() ? 'x' : '-',
            region.filename.empty() ? "[anonymous]" : region.filename
        );
    }
    
    return 0;
}
```
