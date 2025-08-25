# Maps Module Documentation

## Overview

The Maps module provides functionality to read and parse Linux `/proc/[pid]/maps` files, extracting memory region information for process analysis. This is a modern C++20 implementation that replaces the legacy C code with type-safe, RAII-compliant interfaces.

## Module Structure

```cpp
import maps;
```

## Core Components

### 1. Region Types

#### `region_type` Enum

Represents the classification of memory regions:

```cpp
enum class region_type : uint8_t {
    misc,   // Miscellaneous memory regions
    exe,    // Executable binary regions
    code,   // Code segments (shared libraries, etc.)
    heap,   // Heap memory regions
    stack   // Stack memory regions
};

constexpr std::array<std::string_view, 5> region_type_names = {
    "misc", "exe", "code", "heap", "stack"
};
```

### 2. Scan Levels

#### `region_scan_level` Enum

Controls which memory regions are included in scanning:

```cpp
enum class region_scan_level : uint8_t {
    all,                       // All readable regions
    all_rw,                    // All readable/writable regions
    heap_stack_executable,     // Heap, stack, and executable regions
    heap_stack_executable_bss  // Above plus BSS segments
};
```

### 3. Region Metadata

#### `region_flags` Structure

Contains permission and state flags for memory regions:

```cpp
struct region_flags {
    bool read : 1;    // Read permission
    bool write : 1;   // Write permission
    bool exec : 1;    // Execute permission
    bool shared : 1;  // Shared mapping
    bool private_ : 1; // Private mapping
};
```

#### `region` Structure

Complete information about a memory region:

```cpp
struct region {
    void* start;           // Starting address
    std::size_t size;      // Region size in bytes
    region_type type;      // Region classification
    region_flags flags;    // Permission flags
    void* load_addr;       // Load address for ELF files
    std::string filename;  // Associated file path
    std::size_t id;        // Unique identifier
    
    // Helper methods
    [[nodiscard]] bool is_readable() const noexcept;
    [[nodiscard]] bool is_writable() const noexcept;
    [[nodiscard]] bool is_executable() const noexcept;
    [[nodiscard]] bool is_shared() const noexcept;
    [[nodiscard]] bool is_private() const noexcept;
    
    [[nodiscard]] std::pair<void*, std::size_t> as_span() const noexcept;
    [[nodiscard]] bool contains(void* address) const noexcept;
};
```

## Usage Examples

### Basic Usage

```cpp
import maps;

// Read all memory regions from process
auto result = maps::read_process_maps(1234);
if (result) {
    for (const auto& region : *result) {
        std::cout << std::format("Region: {}-{} ({})\n", 
                               region.start, 
                               static_cast<char*>(region.start) + region.size,
                               maps::region_type_names[static_cast<size_t>(region.type)]);
    }
}
```

### Filtered Scanning

```cpp
// Only scan heap and stack regions
auto regions = maps::read_process_maps(
    pid, 
    maps::region_scan_level::heap_stack_executable
);

if (regions) {
    for (const auto& region : *regions) {
        if (region.type == maps::region_type::heap) {
            std::cout << "Heap region found: " << region.filename << "\n";
        }
    }
}
```

### Error Handling

```cpp
auto result = maps::read_process_maps(pid);
if (!result) {
    std::cerr << "Error: " << result.error().message << "\n";
    return;
}
```

## Class: `maps_reader`

### Static Methods

#### `read_process_maps`

Reads memory regions from a process:

```cpp
[[nodiscard]] static std::expected<std::vector<region>, error> 
read_process_maps(pid_t pid, region_scan_level level = region_scan_level::all);
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

### `maps_reader::error` Structure

```cpp
struct error {
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
auto regions = maps::read_process_maps(pid);
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
| `region_t*` | `maps::region` |
| `list_t` | `std::vector<region>` |
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
    
    auto regions = maps::read_process_maps(target_pid);
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
