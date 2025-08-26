# 进程检查模块文档

## 概述

`process_checker` 模块为 NewScanmem 项目提供进程状态监控和检查功能。它通过检查 Linux `/proc` 文件系统来检查进程是否正在运行、已死亡或处于僵尸状态。

## 模块结构

```cpp
export module process_checker;
```

## 依赖项

- `<unistd.h>` - POSIX 操作系统 API
- `<filesystem>` - C++17 文件系统操作
- `<fstream>` - 文件流操作
- `<string>` - 字符串操作

## 核心功能

### 1. 进程状态枚举

```cpp
enum class ProcessState { 
    RUNNING,  // 进程正在运行
    ERROR,    // 检查过程中发生错误
    DEAD,     // 进程不存在
    ZOMBIE    // 进程处于僵尸状态
};
```

### 2. ProcessChecker 类

#### 公共接口

```cpp
class ProcessChecker {
public:
    static ProcessState checkProcess(pid_t pid);
    static bool isProcessDead(pid_t pid);
};
```

#### 方法

##### checkProcess(pid_t pid)

检查指定 PID 的进程状态。

**参数：**

- `pid`: 要检查的进程 ID

**返回值：**

- `ProcessState::RUNNING`: 进程正在运行（包括睡眠、等待、停止状态）
- `ProcessState::ERROR`: 检查过程中发生错误（无效 PID、文件访问问题）
- `ProcessState::DEAD`: 进程不存在（/proc/[pid] 目录缺失）
- `ProcessState::ZOMBIE`: 进程处于僵尸或死亡状态

##### isProcessDead(pid_t pid)

检查进程是否未运行的便捷方法。

**参数：**

- `pid`: 要检查的进程 ID

**返回值：**

- `true`: 进程已死亡、僵尸状态或发生错误
- `false`: 进程正在运行

## 实现细节

### 进程状态检测

模块读取 `/proc/[pid]/status` 并检查 "State:" 字段：

- **运行状态**: 'R' (运行中), 'S' (睡眠), 'D' (等待), 'T' (停止)
- **僵尸状态**: 'Z' (僵尸), 'X' (死亡)
- **错误处理**: 无效状态字符、文件访问错误

### 文件系统操作

1. **验证**: 检查 PID 是否为正数
2. **存在性**: 验证 `/proc/[pid]` 目录是否存在
3. **访问**: 打开 `/proc/[pid]/status` 文件
4. **解析**: 从文件内容中读取状态信息

## 使用示例

### 基本进程检查

```cpp
import process_checker;

pid_t pid = 1234;
ProcessState state = ProcessChecker::checkProcess(pid);
```

### 检查进程是否死亡

```cpp
import process_checker;

pid_t pid = 1234;
if (ProcessChecker::isProcessDead(pid)) {
    std::cout << "进程 " << pid << " 已死亡或不存在" << std::endl;
}
```

### 详细状态检查

```cpp
import process_checker;

pid_t pid = 1234;
ProcessState state = ProcessChecker::checkProcess(pid);

switch (state) {
    case ProcessState::RUNNING:
        std::cout << "进程正在运行" << std::endl;
        break;
    case ProcessState::DEAD:
        std::cout << "进程不存在" << std::endl;
        break;
    case ProcessState::ZOMBIE:
        std::cout << "进程处于僵尸状态" << std::endl;
        break;
    case ProcessState::ERROR:
        std::cout << "检查进程时发生错误" << std::endl;
        break;
}
```

## 错误处理

### 常见错误情况

1. **无效 PID**: 负数或零值 PID
2. **权限不足**: 无法访问 `/proc/[pid]` 目录
3. **文件系统错误**: `/proc` 文件系统不可用
4. **进程不存在**: 指定的 PID 对应的进程不存在

### 错误处理策略

- 返回 `ProcessState::ERROR` 而不是抛出异常
- 记录详细的错误信息用于调试
- 提供便捷方法 `is_process_dead()` 进行简单检查

## 性能考虑

### 优化策略

1. **最小化文件 I/O**: 只读取必要的状态信息
2. **缓存机制**: 避免重复检查同一进程
3. **批量检查**: 支持同时检查多个进程

### 资源使用

- **内存使用**: 最小化内存分配
- **文件描述符**: 及时关闭文件句柄
- **CPU 使用**: 高效的字符串解析

## 线程安全

### 并发访问

- 所有方法都是静态的，不维护状态
- 支持多线程并发调用
- 无共享状态，无需同步机制

### 注意事项

- 进程状态可能在检查过程中发生变化
- 建议在关键操作前重新检查状态
- 考虑使用适当的重试机制

## 扩展性

### 未来增强

1. **更多状态类型**: 支持更详细的进程状态
2. **性能监控**: 添加进程资源使用统计
3. **事件通知**: 支持进程状态变化通知
4. **批量操作**: 优化多进程检查性能
