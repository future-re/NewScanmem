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

- [**中文文档**](./mkdocs/docs/index.md) - 简体中文版本
- [**English Documentation**](./mkdocs/docs/en/index.md) - English version

提交文档命令

```sh
cd mkdocs && mkdocs gh-deploy
```

---

## 🌐 在线文档 | Online Documentation

项目的完整文档已部署至 GitHub Pages，您可以通过以下链接访问：

- [NewScanmem 文档站点](https://future-re.github.io/NewScanmem/)（推荐）

---

## 🚀 快速开始 | Quick Start

### 构建项目 | Build Project

```bash
# 克隆仓库 | Clone repository
git clone https://github.com/future-re/NewScanmem.git
cd NewScanmem

# 创建构建目录 | Create build directory
mkdir build && cd build

# 配置项目 | Configure project
cmake ..

# 构建 | Build
ninja

# 运行 | Run
./NewScanmem
```

### 系统要求 | System Requirements

- **操作系统** | OS: Linux with /proc filesystem
- **编译器** | Compiler: C++23 with modules support (Clang19+,GCC 13+)
- **依赖** | Dependencies: CMake, Boost, libstdc++-13-dev ninja-1.11

### **依赖项**

- **C++ 编译器**：支持 C++20 的编译器（如 GCC 13+、Clang 19+ 或 MSVC 2022+）。
- **CMake**：版本 3.28 或更高。
- **Boost 库**：版本 1.89。

## 📖 使用指南 | Usage Guide

### 构建项目 | Build Project

```bash
# 克隆仓库 | Clone repository
git clone https://github.com/future-re/NewScanmem.git
cd NewScanmem

# 创建构建目录 | Create build directory
mkdir build && cd build

# 配置项目 | Configure project （DENABLE_COVERAGE=ON及开启cov覆盖率生成）
cmake -S . -B build -DCMAKE_CXX_COMPILER=clang++-20 -DENABLE_COVERAGE=ON/OFF -G Ninja

# 构建 | Build
cmake --build build

# （可选）生成cov覆盖率报告
cmake --build build --target coverage

# 运行 | Run
./NewScanmem
```

### 系统要求 | System Requirements

- **操作系统** | OS: Linux with /proc filesystem
- **编译器** | Compiler: C++23 with modules support (Clang19+,GCC 13+)
- **依赖** | Dependencies: CMake, Boost, GTest, libstdc++-13-dev ninja-1.11

