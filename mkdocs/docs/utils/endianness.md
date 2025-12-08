# 字节序模块文档（已简化架构）

## 概述

`utils.endianness` 提供最小且直接的字节序工具：标量字节交换、主机↔网络（大端）转换、主机↔小端转换，以及主机端序查询。

不再提供字节流/结构化布局/variant 的就地转换接口。

## 接口概览

```cpp
// 端序查询
constexpr bool isBigEndian() noexcept;
constexpr bool isLittleEndian() noexcept;

// 通用字节交换（支持 1/2/4/8 字节的可平凡复制类型，含浮点）
template <typename T>
constexpr T swapBytes(T value) noexcept;

// 兼容别名：仅限整数类型
template <typename T>
constexpr T swapBytesIntegral(T value) noexcept;

// 主机 ↔ 网络（大端）
template <typename T>
constexpr T hostToNetwork(T value) noexcept;
template <typename T>
constexpr T networkToHost(T value) noexcept; // 对称操作

// 主机 ↔ 小端
template <typename T>
constexpr T hostToLittleEndian(T value) noexcept;
template <typename T>
constexpr T littleEndianToHost(T value) noexcept; // 对称操作
```

## 使用示例

```cpp
import utils.endianness;

// 字节交换
std::uint32_t v = 0x12345678u;
auto s = swapBytes(v); // 小端系统上 => 0x78563412

// 网络序转换
std::uint16_t port = 8080;
auto netPort = hostToNetwork(port);
auto hostPort = networkToHost(netPort);

// 小端转换（按主机端序判断是否需要交换）
float f = 3.14f;
auto le = hostToLittleEndian(f);
auto back = littleEndianToHost(le);
```

## 说明与约束

- `swapBytes<T>` 与转换函数在编译期使用 `static_assert` 保证 `T` 为可平凡复制且大小为 1/2/4/8 字节。
- 本模块不进行容器/字节流的就地转换，调用方若需处理批量字节，应自行以元素宽度步长迭代并调用标量版本。

## 迁移提示（从旧版）

- 删除：`fixEndianness(Value&, ...)`、字节流/结构化布局/variant 的就地转换系列。
- 保留：`swapBytesIntegral`（作为 `swapBytes` 的整数别名）。
- 推荐：在各自模块中本地处理端序，而非集中处理 `Value`。

## 参见

- [概念：大端与小端](../concepts/endianness.md) — 端序的概念性说明
- [值类型模块](../core/value.md) — `Value` 与 `UserValue`
- [扫描辅助](../scanning_guide.md) — 数值/字节匹配流程
