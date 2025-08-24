# NewScanmem

NewScanmem 是一个使用 C++20 模块重构的现代化版本的 `scanmem`，旨在通过现代 C++ 特性（如模块化、强类型枚举等）提升代码的可读性、可维护性和性能。

[原项目 Scanmem地址](https://github.com/scanmem/scanmem)

---

## 📋 项目简介 | Project Overview

`scanmem` 是一个简单的交互式内存扫描工具，通常用于调试和修改运行中的程序。NewScanmem 在原有功能的基础上，使用 C++20 的模块化特性对代码进行了现代化重构，减少了传统头文件的复杂性，并引入了更高效的字符串处理和内存管理方式。

NewScanmem 是一个现代化的 Linux 内存扫描工具，使用 C++20 模块系统构建，专为进程内存分析和调试设计。

---

## 🚀 特性

- **C++20 模块化**：使用 C++20 的模块特性（`import`），替代传统的头文件机制，提升编译速度和代码组织。
- **现代化代码风格**：引入强类型枚举（`enum class`）、`std::string_view` 等现代 C++ 特性。
- **Boost 支持**：集成 Boost 库（如 `Boost.Filesystem` 和 `Boost.StringAlgo`），增强文件操作和字符串处理能力。
- **高效内存扫描**：优化内存扫描逻辑，提升性能和稳定性。

---

## 🏗️ 文档结构 | Documentation Structure

本项目的文档提供中英双语支持，您可以选择适合的语言查看：

This project provides bilingual documentation in Chinese and English:

- [**中文文档**](./docs/zh/README.md) - 简体中文版本
- [**English Documentation**](./docs/en/README.md) - English version

## 📚 模块文档 | Module Documentation

### 核心模块 | Core Modules

| 模块名称 | 中文文档 | English Documentation | 描述 |
|----------|----------|----------------------|------|
| `endianness` | [字节序模块](./docs/zh/endianness.md) | [Endianness Module](./docs/en/endianness.md) | 字节序检测与转换 |
| `process_checker` | [进程检查模块](./docs/zh/process_checker.md) | [Process Checker Module](./docs/en/process_checker.md) | 进程状态监控 |
| `sets` | [集合模块](./docs/zh/sets.md) | [Sets Module](./docs/en/sets.md) | 集合操作与解析 |
| `show_message` | [消息显示模块](./docs/zh/show_message.md) | [Show Message Module](./docs/en/show_message.md) | 日志与消息系统 |
| `target_mem` | [目标内存模块](./docs/zh/target_mem.md) | [Target Memory Module](./docs/en/target_mem.md) | 内存分析结构 |
| `value` | [值类型模块](./docs/zh/value.md) | [Value Module](./docs/en/value.md) | 数值类型定义 |

### 应用文档 | Application Documentation

| 文档名称 | 中文文档 | English Documentation | 描述 |
|----------|----------|----------------------|------|
| 主应用 | [主应用文档](./docs/zh/main.md) | [Main Application](./docs/en/main.md) | 主程序入口与架构 |
| API参考 | [API参考](./docs/zh/API_REFERENCE.md) | [API Reference](./docs/en/API_REFERENCE.md) | 完整API文档 |

### 开发规范 | Development Guidelines

| 文档名称 | 描述 |
|----------|------|
| [命名规范](./docs/NamingConvention.md) | C++代码命名规范 |

---

## 🚀 快速开始 | Quick Start

### 构建项目 | Build Project

```bash
# 克隆仓库 | Clone repository
git clone https://github.com/your-org/newscanmem.git
cd newscanmem

# 创建构建目录 | Create build directory
mkdir build && cd build

# 配置项目 | Configure project
cmake ..

# 构建 | Build
make

# 运行 | Run
./newscanmem
```

### 系统要求 | System Requirements

- **操作系统** | OS: Linux with /proc filesystem
- **编译器** | Compiler: C++23 with modules support (GCC 13+)
- **依赖** | Dependencies: CMake, Boost, libstdc++-13-dev

### **依赖项**

- **C++ 编译器**：支持 C++20 的编译器（如 GCC 12+、Clang 18+ 或 MSVC 2022+）。
- **CMake**：版本 3.28 或更高。
- **Boost 库**：版本 1.89。

## 📖 使用指南 | Usage Guide

### 选择语言 | Choose Language

- **中文用户** | Chinese users: [开始阅读](./docs/zh/README.md)
- **English users**: [Get started](./docs/en/README.md)

## 🗂️ 目录结构 | Directory Structure

```tree
docs/
├── README.md              # 本文档 | This document
├── NamingConvention.md    # 命名规范 | Naming conventions
├── zh/                    # 中文文档 | Chinese docs
│   ├── README.md
│   ├── endianness.md
│   ├── process_checker.md
│   ├── sets.md
│   ├── show_message.md
│   ├── target_mem.md
│   ├── value.md
│   ├── main.md
│   └── API_REFERENCE.md
├── en/                    # 英文文档 | English docs
│   ├── README.md
│   ├── endianness.md
│   ├── process_checker.md
│   ├── sets.md
│   ├── show_message.md
│   ├── target_mem.md
│   ├── value.md
│   ├── main.md
│   └── API_REFERENCE.md
└── assets/               # 资源文件 | Assets (future)
```

## 📞 支持 | Support

### 选择支持语言 | Choose Support Language

- **中文支持** | Chinese support: [查看中文文档](./docs/zh/README.md)
- **English support**: [View English docs](./docs/en/README.md)

---

**立即开始 | Get Started:** [中文](./docs/zh/README.md) | [English](./docs/en/README.md)
