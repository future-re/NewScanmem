# NewScanmem Documentation - English Version

## Project Overview

NewScanmem is a modern C++23 memory scanning utility that provides comprehensive process memory scanning and analysis capabilities, supporting various data types and endianness handling.

## 📂 Documentation Structure

### Module Documentation

- [Endianness Module](endianness.md) - Byte order and endianness utilities
- [Process Checker Module](process_checker.md) - Process state monitoring and checking
- [Sets Module](sets.md) - Set operations and parsing utilities
- [Show Message Module](show_message.md) - Message printing and logging system
- [Target Memory Module](target_mem.md) - Memory matching and analysis structures
- [Value Module](value.md) - Value type definitions and match flags
- [Main Application](main.md) - Entry point and application flow

### Quick Reference

- [API Reference Manual](API_REFERENCE.md) - Complete API documentation

## 🚀 Quick Start

### Build Requirements

- C++23 compiler with modules support
- Boost libraries (regex, spirit)
- Linux environment (requires /proc filesystem)

### Basic Usage

```bash
# Build project
mkdir build && cd build
cmake ..
make

# Run (current version is placeholder)
./newscanmem
```

## 🏗️ Architecture Overview

Codebase is organized into C++23 modules, each providing specific functionality:

1. **Core Types**: Value representations and match flags
2. **Process Management**: Process state checking and monitoring
3. **Memory Operations**: Memory scanning and analysis
4. **Utilities**: Endianness handling, message systems, and set operations

## 📋 Features

### Supported Data Types

- Integers: int8/16/32/64, uint8/16/32/64
- Floats: float32, float64
- Byte arrays and strings
- Wildcard patterns

### Core Features

- Process memory scanning
- Multi-type value matching
- Automatic endianness handling
- Memory region filtering
- Result set management

## 📖 Usage Guide

### Process Checking

```cpp
import process_checker;

pid_t pid = 1234;
auto state = ProcessChecker::check_process(pid);
if (state == ProcessState::RUNNING) {
    // Process is running
}
```

### Set Parsing

```cpp
import sets;

Set mySet;
if (parse_uintset("1,2,3,10..15", mySet, 100)) {
    // Use parsed set
}
```

### Memory Analysis

```cpp
import targetmem;

MatchesAndOldValuesArray matches(1024);
// Perform memory scanning...
```

## 🔧 Development Guide

### Module Integration

```cpp
// Example: Integrating multiple modules
import process_checker;
import targetmem;
import show_message;

void scan_process(pid_t pid) {
    MessagePrinter printer;
    
    if (ProcessChecker::is_process_dead(pid)) {
        printer.error("Process {} is not running", pid);
        return;
    }
    
    printer.info("Starting scan for process {}...", pid);
    // Perform scanning...
}
```

## 🔍 Debugging and Testing

### Debug Build

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### Logging System

```cpp
MessageContext ctx{.debugMode = true};
MessagePrinter printer(ctx);
printer.debug("Debug info: {}", some_value);
```

## 📱 Cross-Platform Support

- **Linux**: Full support (primary platform)
- **macOS**: Planned support
- **Windows**: Not currently supported (requires different implementation)

## 🤝 Contributing

### Development Setup

```bash
# Install dependencies
sudo apt install build-essential cmake libboost-all-dev

# Clone and build
git clone [repository-url]
cd NewScanmem
mkdir build && cd build
cmake ..
make test
```

## 📄 License

This project is open source and follows standard open source licensing. See individual module documentation for specific information.

## 🔗 Related Links

- [Chinese Documentation](../zh/README.md)
- [API Reference](API_REFERENCE.md)
- [GitHub Repository](https://github.com/your-org/newscanmem)
