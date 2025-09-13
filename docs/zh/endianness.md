# 字节序模块文档

## 概述

`endianness` 模块为 NewScanmem 项目提供全面的字节序处理工具。它支持编译时和运行时字节序检测、字节交换操作，以及各种数据类型的自动字节序校正。

## 模块结构

```cpp
export module endianness;
```

## 依赖项

- `<cstdint>` - 定宽整数类型
- `<cstring>` - C字符串操作
- `<bit>` - 位操作和字节序检测
- `<type_traits>` - 模板元编程的类型特征
- `<concepts>` - C++20概念
- `value` 模块 - 值类型定义

## 核心功能

### 1. 字节序检测

#### 编译时检测

```cpp
constexpr bool isBigEndian() noexcept;
constexpr bool isLittleEndian() noexcept;
```

使用 `std::endian::native` 在编译时确定主机字节序。

### 2. 字节交换函数

#### 基本字节交换函数

```cpp
constexpr uint8_t swapBytes(uint8_t value) noexcept;
constexpr uint16_t swapBytes(uint16_t value) noexcept;
constexpr uint32_t swapBytes(uint32_t value) noexcept;
constexpr uint64_t swapBytes(uint64_t value) noexcept;
```

#### 通用字节交换函数

```cpp
template<typename T>
constexpr T swapBytesIntegral(T value) noexcept;
```

支持大小为1、2、4和8字节的整数类型。

### 3. 值类型字节序校正函数

```cpp
void fixEndianness(Value& value, bool reverseEndianness) noexcept;
```

对 `Value` 的 `bytes` 进行就地字节序校正（根据 `flags` 推断宽度 2/4/8）。

### 4. 网络字节序转换函数

```cpp
template<SwappableIntegral T>
constexpr T hostToNetwork(T value) noexcept;

template<SwappableIntegral T>
constexpr T networkToHost(T value) noexcept;
```

在主机和网络字节序（大端）之间转换。

### 5. 小端转换函数

```cpp
template<SwappableIntegral T>
constexpr T hostToLittleEndian(T value) noexcept;

template<SwappableIntegral T>
constexpr T littleEndianToHost(T value) noexcept;
```

## 使用示例

### 基本字节交换

```cpp
import endianness;

uint32_t value = 0x12345678;
uint32_t swapped = endianness::swapBytes(value);
// 在小端系统上 swapped = 0x78563412
```

### 字节序校正

```cpp
import endianness;
import value;

Value val = uint32_t{0x12345678};
endianness::fixEndianness(val, true);  // 反转字节序
```

### 网络通信

```cpp
uint16_t port = 8080;
uint16_t networkPort = endianness::hostToNetwork(port);
```

## 概念和约束

### SwappableIntegral 概念

```cpp
template<typename T>
concept SwappableIntegral = std::integral<T> && 
    (sizeof(T) == 1 || sizeof(T) == 2 || sizeof(T) == 4 || sizeof(T) == 8);
```

将字节交换操作限制为特定大小的整数类型。

## 实现细节

### 字节交换算法

- **16位**：使用位旋转：`(value << 8) | (value >> 8)`
- **32位**：使用位掩码和移位以获得最佳性能
- **64位**：跨8字节使用位掩码和移位

### 编译时优化

所有字节交换操作都标记为 `constexpr`，以便在可能的情况下进行编译时求值。

### 类型安全

使用C++20概念确保类型安全，并为不支持的类型提供清晰的错误消息。

## 错误处理

- `swapBytesIntegral` 使用 `static_assert` 进行编译时类型检查
- `swapBytesInPlace` 静默忽略不支持的尺寸
- `fixEndianness` 按 `flags` 推断宽度（B16/B32/B64）对 `Value.bytes` 原地交换

## 性能考虑

- 所有操作都是 constexpr 用于编译时优化
- 字节交换使用高效的位操作
- 无动态内存分配
- 对支持类型的最小运行时开销

## 参见

- [值类型模块](value.md) - 值类型定义
- [目标内存模块](target_mem.md) - 内存分析操作
