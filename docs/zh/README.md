# NewScanmem 文档 - 中文版

## 项目概述

NewScanmem 是一个基于现代 C++23 开发的内存扫描工具，提供进程内存扫描和分析功能，支持多种数据类型和字节序处理。

## 📂 文档结构

### 模块文档

- [字节序模块](endianness.md) - 字节序和字节交换工具
- [进程检查模块](process_checker.md) - 进程状态监控和检查
- [集合模块](sets.md) - 集合操作和解析工具
- [消息显示模块](show_message.md) - 消息打印和日志系统
- [目标内存模块](target_mem.md) - 内存匹配和分析结构
- [值类型模块](value.md) - 值类型和匹配标志定义
- [主应用程序](main.md) - 入口点和应用程序流程

### 快速参考

- [API参考手册](API_REFERENCE.md) - 完整的API文档

## 🚀 快速开始

### 构建要求

- 支持C++23的编译器（带模块支持）
- Boost库（regex, spirit）
- Linux环境（需要/proc文件系统支持）

### 基本用法

```bash
# 构建项目
mkdir build && cd build
cmake ..
make

# 运行（当前版本为占位符）
./newscanmem
```

## 🏗️ 架构概述

代码库按C++23模块组织，每个模块提供特定功能：

1. **核心类型**：值表示和匹配标志
2. **进程管理**：进程状态检查和监控
3. **内存操作**：内存扫描和分析
4. **工具**：字节序处理、消息系统和集合操作

## 📋 功能特性

### 支持的数据类型

- 整数：int8/16/32/64, uint8/16/32/64
- 浮点数：float32, float64
- 字节数组和字符串
- 通配符模式

### 核心功能

- 进程内存扫描
- 多数据类型匹配
- 字节序自动处理
- 内存区域过滤
- 结果集合管理

## 📖 使用指南

### 进程检查

```cpp
import process_checker;

pid_t pid = 1234;
auto state = ProcessChecker::check_process(pid);
if (state == ProcessState::RUNNING) {
    // 进程正在运行
}
```

### 集合解析

```cpp
import sets;

Set mySet;
if (parse_uintset("1,2,3,10..15", mySet, 100)) {
    // 使用解析后的集合
}
```

### 内存分析

```cpp
import targetmem;

MatchesAndOldValuesArray matches(1024);
// 执行内存扫描...
```

## 🔧 开发指南

### 模块集成

```cpp
// 示例：集成多个模块
import process_checker;
import targetmem;
import show_message;

void scan_process(pid_t pid) {
    MessagePrinter printer;
    
    if (ProcessChecker::is_process_dead(pid)) {
        printer.error("进程 {} 未运行", pid);
        return;
    }
    
    printer.info("开始扫描进程 {}...", pid);
    // 执行扫描...
}
```

## 🔍 调试和测试

### 调试构建

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
```

### 日志系统

```cpp
MessageContext ctx{.debugMode = true};
MessagePrinter printer(ctx);
printer.debug("调试信息: {}", some_value);
```

## 📱 跨平台支持

- **Linux**: 完全支持（主要平台）
- **macOS**: 计划中支持
- **Windows**: 暂不支持（需要不同实现）

## 🤝 贡献指南

### 开发环境设置

```bash
# 安装依赖
sudo apt install build-essential cmake libboost-all-dev

# 克隆和构建
git clone [repository-url]
cd NewScanmem
mkdir build && cd build
cmake ..
make test
```

## 📄 许可证

本项目开源，遵循标准开源许可证。具体信息请查看各模块文档。

## 🔗 相关链接

- [英文文档](../en/README.md)
- [API参考](API_REFERENCE.md)
- [GitHub仓库](https://github.com/future-re/NewScanmem)

## 🖥️ 命令行界面 (CLI)

### 概述
NewScanmem 的 CLI 提供了一个交互式 REPL（读取-求值-打印循环）环境，用于内存扫描和分析。用户可以通过执行命令与系统交互，例如扫描内存、管理结果等。

### 可用命令
- `help`：显示可用命令。
- `quit`：退出 CLI。
- `pid`：显示当前进程 ID。

### 示例用法
```bash
# 启动 CLI
./newscanmem

# 在 CLI 中
> help
可用命令：help, quit, pid
> pid
当前 PID: 1234
> quit
退出...
```
