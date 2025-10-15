# 消息显示模块文档

## 概述

`show_message` 模块为 NewScanmem 项目提供全面的消息打印和日志系统。它支持多种消息类型、基于调试/后端模式的条件输出，以及使用 C++20 格式字符串的格式化输出。

## 模块结构

```cpp
export module show_message;
```

## 依赖项

- `<boost/process.hpp>` - Boost 进程库
- `<cstdint>` - 固定宽度整数类型
- `<format>` - C++20 格式库
- `<iostream>` - 标准 I/O 流
- `<string_view>` - 用于高效字符串处理的字符串视图

## 核心功能

### 1. 消息类型系统

```cpp
enum class MessageType : uint8_t {
    INFO,    // 信息消息
    WARN,    // 警告消息
    ERROR,   // 错误消息
    DEBUG,   // 调试消息（条件性）
    USER     // 面向用户的消息（条件性）
};
```

### 2. 消息上下文

```cpp
struct MessageContext {
    bool debugMode = false;    // 启用调试输出
    bool backendMode = false;  // 抑制用户输出
};
```

### 3. MessagePrinter 类

#### 构造函数

```cpp
MessagePrinter(MessageContext ctx = {});
```

#### 主要打印方法

```cpp
template<typename... Args>
void print(MessageType type, std::string_view fmt, Args&&... args) const;
```

#### 便捷方法

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

## 使用示例

### 基本使用

```cpp
import show_message;

MessagePrinter printer;
printer.info("开始内存扫描");
printer.warn("检测到内存不足");
printer.error("无法打开进程");
```

### 使用格式字符串

```cpp
printer.info("正在扫描 PID 为 {} 的进程", pid);
printer.warn("发现 {} 个可疑内存区域", suspicious_count);
printer.error("在地址 0x{:08x} 处内存读取失败", address);
```

### 调试模式使用

```cpp
MessageContext ctx;
ctx.debugMode = true;
MessagePrinter debugPrinter(ctx);

debugPrinter.debug("调试信息: 内存地址 0x{:x}", address);
debugPrinter.debug("内部状态: {}", internal_state);
```

### 后端模式使用

```cpp
MessageContext ctx;
ctx.backendMode = true;
MessagePrinter backendPrinter(ctx);

// 用户消息将被抑制
backendPrinter.user("这条消息不会显示");
// 但错误消息仍然会显示
backendPrinter.error("错误消息仍然会显示");
```

## 消息类型详解

### INFO 消息

- **用途**: 一般信息性消息
- **显示条件**: 始终显示
- **示例**: 程序启动、扫描完成、状态更新

```cpp
printer.info("内存扫描完成");
printer.info("找到 {} 个匹配项", match_count);
```

### WARN 消息

- **用途**: 警告信息，需要注意但不致命
- **显示条件**: 始终显示
- **示例**: 性能警告、配置问题、非关键错误

```cpp
printer.warn("内存使用率较高: {}%", memory_usage);
printer.warn("某些区域无法访问");
```

### ERROR 消息

- **用途**: 错误信息，表示操作失败
- **显示条件**: 始终显示
- **示例**: 文件打开失败、权限错误、系统错误

```cpp
printer.error("无法打开进程: {}", error_message);
printer.error("内存读取失败: {}", strerror(errno));
```

### DEBUG 消息

- **用途**: 调试信息，用于开发调试
- **显示条件**: 仅在 debugMode = true 时显示
- **示例**: 内部状态、详细执行流程、性能数据

```cpp
printer.debug("内部状态: {}", internal_state);
printer.debug("执行时间: {} ms", execution_time);
```

### USER 消息

- **用途**: 面向用户的消息
- **显示条件**: 仅在 backendMode = false 时显示
- **示例**: 用户提示、进度信息、交互消息

```cpp
printer.user("请输入目标进程 ID:");
printer.user("扫描进度: {}%", progress);
```

## 格式化功能

### C++20 格式字符串

支持完整的 C++20 格式字符串功能：

```cpp
// 整数格式化
printer.info("进程 ID: {}", pid);
printer.info("地址: 0x{:08x}", address);

// 浮点数格式化
printer.info("内存使用率: {:.2f}%", memory_usage);

// 字符串格式化
printer.info("文件名: {}", filename);

// 复杂格式化
printer.info("扫描结果: {} 个匹配项，耗时 {:.3f} 秒", 
            match_count, elapsed_time);
```

### 类型安全

- 编译时类型检查
- 自动类型推导
- 防止格式化错误

## 上下文管理

### 全局上下文

```cpp
// 创建全局消息打印机
MessageContext globalContext;
globalContext.debugMode = true;
MessagePrinter globalPrinter(globalContext);
```

### 局部上下文

```cpp
void someFunction() {
    MessageContext localContext;
    localContext.backendMode = true;
    MessagePrinter localPrinter(localContext);
    
    // 使用局部打印机
    localPrinter.info("局部消息");
}
```

### 上下文继承

```cpp
class MyClass {
private:
    MessagePrinter m_printer;
    
public:
    MyClass(MessageContext ctx) : m_printer(ctx) {}
    
    void doSomething() {
        m_printer.info("类内部消息");
    }
};
```

## 性能考虑

### 编译时优化

- 模板实例化优化
- 内联函数调用
- 条件编译

### 运行时优化

- 字符串视图避免拷贝
- 格式化参数完美转发
- 最小化内存分配

### 输出控制

- 条件输出减少 I/O 开销
- 调试模式可完全禁用
- 后端模式抑制用户输出

## 错误处理

### 格式化错误

- 编译时检查格式字符串
- 运行时验证参数类型
- 优雅的错误处理

### 输出错误

- 检查输出流状态
- 处理写入失败
- 提供错误反馈

## 扩展性

### 自定义消息类型

```cpp
enum class CustomMessageType : uint8_t {
    VERBOSE,  // 详细输出
    TRACE,    // 跟踪信息
    // ... 其他类型
};
```

### 自定义输出目标

```cpp
class CustomPrinter : public MessagePrinter {
public:
    void print(MessageType type, std::string_view fmt, auto&&... args) const override {
        // 自定义输出逻辑
    }
};
```

### 日志文件支持

```cpp
class FileLogger {
private:
    std::ofstream m_logFile;
    MessagePrinter m_printer;
    
public:
    FileLogger(const std::string& filename) : m_logFile(filename) {
        // 配置文件输出
    }
    
    void log(MessageType type, std::string_view fmt, auto&&... args) {
        // 写入文件
        m_printer.print(type, fmt, std::forward<decltype(args)>(args)...);
    }
};
```
