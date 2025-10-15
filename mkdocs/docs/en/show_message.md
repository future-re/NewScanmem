# Show Message Module Documentation

## Overview

The `show_message` module provides a comprehensive message printing and logging system for the NewScanmem project. It supports multiple message types, conditional output based on debug/backend modes, and formatted output using C++20 format strings.

## Module Structure

```cpp
export module show_message;
```

## Dependencies

- `<boost/process.hpp>` - Boost process library
- `<cstdint>` - Fixed-width integer types
- `<format>` - C++20 format library
- `<iostream>` - Standard I/O streams
- `<string_view>` - String view for efficient string handling

## Core Features

### 1. Message Type System

```cpp
enum class MessageType : uint8_t {
    INFO,    // Informational messages
    WARN,    // Warning messages
    ERROR,   // Error messages
    DEBUG,   // Debug messages (conditional)
    USER     // User-facing messages (conditional)
};
```

### 2. Message Context

```cpp
struct MessageContext {
    bool debugMode = false;    // Enable debug output
    bool backendMode = false;  // Suppress user output
};
```

### 3. MessagePrinter Class

#### Constructor

```cpp
MessagePrinter(MessageContext ctx = {});
```

#### Main Print Method

```cpp
template<typename... Args>
void print(MessageType type, std::string_view fmt, Args&&... args) const;
```

#### Convenience Methods

```cpp
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
```

## Usage Examples

### Basic Usage

```cpp
import show_message;

MessagePrinter printer;
printer.info("Starting memory scan");
printer.warn("Low memory detected");
printer.error("Failed to open process");
```

### With Format Strings

```cpp
printer.info("Scanning process with PID: {}", pid);
printer.warn("Found {} suspicious memory regions", suspicious_count);
printer.error("Memory read failed at address: 0x{:08x}", address);
```

### Debug Mode Usage

```cpp
MessageContext ctx{.debugMode = true};
MessagePrinter debug_printer(ctx);

debug_printer.debug("Current memory usage: {} bytes", memory_usage);
debug_printer.debug("Processing region: 0x{:08x}-0x{:08x}", start, end);
```

### Backend Mode Usage

```cpp
MessageContext ctx{.backendMode = true};
MessagePrinter backend_printer(ctx);

backend_printer.user("Scan complete");  // Won't print in backend mode
backend_printer.info("Scan complete");  // Will print to stderr
```

### Combined Context

```cpp
MessageContext ctx{
    .debugMode = true,
    .backendMode = false
};
MessagePrinter printer(ctx);

printer.debug("Debug info");  // Prints to stderr
printer.user("User message"); // Prints to stdout
```

## Output Channels

### INFO, WARN, ERROR, DEBUG

- **Output**: `std::cerr`
- **Format**: `[level]: message`

### USER

- **Output**: `std::cout` (unless backend mode is enabled)
- **Format**: Raw message without prefix

## Message Formatting

The module uses C++20 format strings with the following features:

### Supported Format Specifiers

```cpp
printer.info("Integer: {}", 42);
printer.info("Hex: 0x{:08x}", 0x1234);
printer.info("Float: {:.2f}", 3.14159);
printer.info("String: {}", "hello");
printer.info("Multiple: {} and {}", 1, "two");
```

### Error Handling

Format string errors are handled by the standard format library and will throw appropriate exceptions.

## Integration Examples

### With Other Modules

```cpp
import show_message;
import process_checker;

void check_and_report(pid_t pid) {
    MessagePrinter printer;
    
    auto state = ProcessChecker::check_process(pid);
    switch (state) {
        case ProcessState::RUNNING:
            printer.info("Process {} is running", pid);
            break;
        case ProcessState::DEAD:
            printer.warn("Process {} is dead", pid);
            break;
        case ProcessState::ERROR:
            printer.error("Error checking process {}", pid);
            break;
    }
}
```

### Configuration-based Usage

```cpp
struct Config {
    bool debug;
    bool backend;
};

void initialize_logging(const Config& config) {
    MessageContext ctx{
        .debugMode = config.debug,
        .backendMode = config.backend
    };
    
    static MessagePrinter printer(ctx);
    // Use printer throughout application
}
```

## Thread Safety

The `MessagePrinter` class is thread-safe for concurrent use. Each instance maintains its own context and can be used from multiple threads simultaneously.

## Performance Considerations

- **Format String Parsing**: Done at compile time for literal strings
- **Memory Allocation**: Minimal heap allocation for formatted strings
- **I/O Buffering**: Uses standard stream buffering

## Customization

### Extending Message Types

To add new message types, extend the `MessageType` enum and add appropriate handling in the `print` method:

```cpp
// Example extension
enum class ExtendedMessageType {
    VERBOSE,  // More detailed than DEBUG
    TRACE     // Function tracing
};
```

### Custom Output Redirection

While the module uses fixed output streams, you can create wrapper functions:

```cpp
class CustomMessagePrinter : public MessagePrinter {
    std::ofstream log_file;
    
public:
    void custom_log(std::string_view msg) {
        log_file << std::format("{}\n", msg);
        info("Logged: {}", msg);
    }
};
```

## Error Handling Module

### Exception Safety

All methods provide strong exception safety guarantee. Format string errors are propagated to the caller.

### Common Issues

1. **Format String Mismatches**: Ensure format specifiers match argument types
2. **Locale Issues**: Consider locale settings for number formatting
3. **Buffer Overflow**: Not possible with C++20 format library

## Testing

```cpp
void test_message_printer() {
    MessagePrinter printer;
    
    printer.info("Test info message");
    printer.warn("Test warning message");
    printer.error("Test error message");
    
    MessageContext ctx{.debugMode = true};
    MessagePrinter debug_printer(ctx);
    debug_printer.debug("Test debug message");
    
    ctx.backendMode = true;
    MessagePrinter backend_printer(ctx);
    backend_printer.user("Test user message");  // Won't print
}
```

## See Also

- [Target Memory Module](target_mem.md) - Uses message printing for memory operations
- [Process Checker Module](process_checker.md) - Uses messages for process state reporting
- [Main Application](main.md) - Integration in main application flow

## Future Enhancements

- Log file support
- Colored output support
- Custom log levels
- Timestamp support
- Log rotation capabilities
- JSON format output option
- Syslog integration
- Windows Event Log support (cross-platform)

## Platform Notes

- **Linux/macOS**: Full support for all features
- **Windows**: Full support for all features
- **UTF-8**: Supports Unicode messages on all platforms
- **Terminal Colors**: Future enhancement for colorized output

## Best Practices

1. **Use appropriate message types** for different severity levels
2. **Enable debug mode** only when needed to reduce noise
3. **Use backend mode** for automated/scripted operations
4. **Format strings**: Use appropriate format specifiers for readability
5. **Error handling**: Always handle format string errors appropriately
6. **Performance**: Avoid complex formatting in tight loops

## Migration from printf-style logging

```cpp
// Old style
printf("Error: %s at line %d\n", message, line);

// New style
printer.error("{} at line {}", message, line);
```

## Integration with Build Systems

The module can be configured through build flags:

```cpp
#ifdef NDEBUG
    MessageContext ctx{.debugMode = false};
#else
    MessageContext ctx{.debugMode = true};
#endif
```

## Common Patterns

### Scoped Debugging

```cpp
class ScopedDebug {
    MessagePrinter& printer;
    std::string name;
    
public:
    ScopedDebug(MessagePrinter& p, std::string_view n) 
        : printer(p), name(n) {
        printer.debug("Entering {}", name);
    }
    
    ~ScopedDebug() {
        printer.debug("Exiting {}", name);
    }
};

// Usage
{
    ScopedDebug debug(printer, "memory_scan");
    // ... scan memory ...
} // Automatically logs exit
```
