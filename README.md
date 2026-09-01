# memseek

memseek 是一个使用现代 C++ 重构的内存扫描工具，公共接口位于 `include/memseek/`，旨在通过强类型和稳定的头文件接口提升可读性、可维护性和集成能力。

[原项目 Scanmem地址](https://github.com/scanmem/scanmem)

---

## 📋 项目简介 | Project Overview

`scanmem` 是一个简单的交互式内存扫描工具，通常用于调试和修改运行中的程序。memseek 在原有功能的基础上，使用 C++23 和普通 `.hpp` 接口进行重构，并引入了更高效的字符串处理和内存管理方式。

memseek 是一个现代化的 Linux 内存扫描工具，专为进程内存分析和调试设计。

---

## 🚀 特性

- **稳定头文件接口**：公共头文件位于 `include/memseek/`，不依赖 C++ Modules 或 BMI 缓存。
- **现代化代码风格**：引入强类型枚举（`enum class`）、`std::string_view` 等现代 C++ 特性。
- **标准库实现**：正则、集合解析和文件操作使用 C++ 标准库，减少外部依赖。
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

- [memseek 文档站点](https://future-re.github.io/Memseek/)（推荐）

---

## 🚀 快速开始 | Quick Start

### 构建项目 | Build Project

```bash
# 克隆仓库 | Clone repository
git clone https://github.com/future-re/Memseek.git
cd memseek

# 创建构建目录 | Create build directory
mkdir build && cd build

# 配置项目 | Configure project
cmake ..

# 构建 | Build
ninja

# 运行 | Run
./memseek
```

### 系统要求 | System Requirements

- **操作系统** | OS: Linux with /proc filesystem
- **编译器** | Compiler: C++23（GCC 13+、Clang 16+）
- **依赖** | Dependencies: CMake, libstdc++-13-dev

### **依赖项**

- **C++ 编译器**：支持 C++23 的编译器（如 GCC 13+、Clang 16+ 或 MSVC 2022+）。
- **CMake**：版本 3.28 或更高。
- **第三方库**：ICU（运行时字符串/国际化支持）。

## 📖 使用指南 | Usage Guide

### 构建项目 | Build Project

```bash
# 克隆仓库 | Clone repository
git clone https://github.com/future-re/Memseek.git
cd memseek

# 创建构建目录 | Create build directory
mkdir build && cd build

# 配置项目 | Configure project （DENABLE_COVERAGE=ON及开启cov覆盖率生成）
cmake -S . -B build -DCMAKE_CXX_COMPILER=clang++-20 -DENABLE_COVERAGE=ON/OFF -G Ninja

# 构建 | Build
cmake --build build

# （可选）生成cov覆盖率报告
./generate_coverage.sh

# 运行 | Run
./memseek
```

### 系统要求 | System Requirements

- **操作系统** | OS: Linux with /proc filesystem
- **编译器** | Compiler: C++23（GCC 13+、Clang 16+）
- **依赖** | Dependencies: CMake, GTest, ICU, libstdc++-13-dev
