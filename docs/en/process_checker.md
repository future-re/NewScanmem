# Process Checker Module Documentation

## Overview

The `process_checker` module provides process state monitoring and checking functionality for the NewScanmem project. It allows checking if processes are running, dead, or in zombie states by examining the Linux `/proc` filesystem.

## Module Structure

```cpp
export module process_checker;
```

## Dependencies

- `<unistd.h>` - POSIX operating system API
- `<filesystem>` - C++17 filesystem operations
- `<fstream>` - File stream operations
- `<string>` - String operations

## Core Features

### 1. Process State Enumeration

```cpp
enum class ProcessState { 
    RUNNING,  // Process is running
    ERROR,    // Error occurred during checking
    DEAD,     // Process does not exist
    ZOMBIE    // Process is in zombie state
};
```

### 2. ProcessChecker Class

#### Public Interface

```cpp
class ProcessChecker {
public:
    static ProcessState checkProcess(pid_t pid);
    static bool isProcessDead(pid_t pid);
};
```

#### Methods

##### checkProcess(pid_t pid)

Checks the state of a process given its PID.

**Parameters:**

- `pid`: Process ID to check

**Returns:**

- `ProcessState::RUNNING`: Process is running (includes sleeping, waiting, stopped states)
- `ProcessState::ERROR`: Error occurred during checking (invalid PID, file access issues)
- `ProcessState::DEAD`: Process does not exist (/proc/[pid] directory missing)
- `ProcessState::ZOMBIE`: Process is in zombie or dead state

##### isProcessDead(pid_t pid)

Convenience method to check if a process is not running.

**Parameters:**

- `pid`: Process ID to check

**Returns:**

- `true`: Process is dead, zombie, or error occurred
- `false`: Process is running

## Implementation Details

### Process State Detection

The module reads `/proc/[pid]/status` and examines the "State:" field:

- **Running states**: 'R' (Running), 'S' (Sleeping), 'D' (Waiting), 'T' (Stopped)
- **Zombie states**: 'Z' (Zombie), 'X' (Dead)
- **Error handling**: Invalid state characters, file access errors

### File System Operations

1. **Validation**: Checks if PID is positive
2. **Existence**: Verifies `/proc/[pid]` directory exists
3. **Access**: Opens `/proc/[pid]/status` file
4. **Parsing**: Reads state information from file content

## Usage Examples

### Basic Process Checking

```cpp
import process_checker;

pid_t pid = 1234;
ProcessState state = ProcessChecker::checkProcess(pid);

switch (state) {
    case ProcessState::RUNNING:
        std::cout << "Process is running\n";
        break;
    case ProcessState::DEAD:
        std::cout << "Process does not exist\n";
        break;
    case ProcessState::ZOMBIE:
        std::cout << "Process is zombie/dead\n";
        break;
    case ProcessState::ERROR:
        std::cout << "Error checking process\n";
        break;
}
```

### Quick Process Death Check

```cpp
if (ProcessChecker::isProcessDead(pid)) {
    std::cout << "Process is no longer running\n";
} else {
    std::cout << "Process is still running\n";
}
```

### Monitoring Loop

```cpp
#include <chrono>
#include <thread>

void monitorProcess(pid_t pid) {
    while (true) {
        if (ProcessChecker::isProcessDead(pid)) {
            std::cout << "Process " << pid << " has terminated\n";
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
```

## Error Handling

### Error Conditions

1. **Invalid PID**: PIDs less than or equal to 0 return `ProcessState::ERROR`
2. **File Access**: Cannot open `/proc/[pid]/status` returns `ProcessState::ERROR`
3. **Parse Errors**: Malformed state information returns `ProcessState::ERROR`

### Linux Specific Notes

- Requires Linux `/proc` filesystem
- Needs read permissions on `/proc/[pid]/status`
- May not work on systems with restricted `/proc` access

## Performance Considerations

- File I/O operations may have some overhead
- Consider caching results for frequent checks
- Thread-safe for concurrent access to different PIDs

## Security Considerations

- No elevated privileges required for basic checking
- May require appropriate permissions to read `/proc/[pid]/status`
- Does not perform any process memory access

## Platform Compatibility

- **Linux**: Full support (primary target platform)
- **macOS**: Not supported (different proc filesystem structure)
- **Windows**: Not supported (requires different implementation)

## See Also

- [Target Memory Module](target_mem.md) - For memory analysis operations
- [Main Application](main.md) - For process targeting in the application

## Future Enhancements

- Support for additional process information
- Cross-platform compatibility
- Non-blocking process checking
- Process permission checking
