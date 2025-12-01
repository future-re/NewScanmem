# 值类型模块文档

## 概述

`value` 模块提供两类核心实体：
- `Value`：用于保存“旧值”的原始字节和宽度标志（匹配时参考）。
- `UserValue`：用于表达用户输入的数值或范围（包含各标量字段与便捷构造）。

并提供 `MatchFlags` 用于标注匹配宽度：`B8/B16/B32/B64`。

## 模块结构

```cpp
export module value;
export import value.flags;
export import value.scalar;
```

## 匹配标志（MatchFlags）

```cpp
enum class MatchFlags : uint16_t {
    EMPTY = 0,
    B8 = 1u << 0,
    B16 = 1u << 1,
    B32 = 1u << 2,
    B64 = 1u << 3,
};
```

说明：仅保留与匹配宽度直接相关的最小集合，便于明确比较语义。

## 结构体：Value（旧值快照）

Value 仅负责保存历史字节与宽度标志，不再承担类型推断或端序转换。

```cpp
struct Value {
    MatchFlags flags = MatchFlags::EMPTY;
    std::vector<std::uint8_t> bytes;

    void setBytes(const std::uint8_t* data, std::size_t len);
    void setBytes(const std::vector<std::uint8_t>& data);

    [[nodiscard]] const std::uint8_t* data() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
};
```

要点：
- 数值严格比较由上层根据 `flags` 与期望类型宽度自行校验。
- 字节/字符串匹配直接基于 `bytes` 执行，不依赖 `flags`。

## 结构体：UserValue（用户输入）

UserValue 提供各标量的低/高端字段（用于范围），并提供便捷构造：

```cpp
struct UserValue {
    // 标量字段（low/high）
    int8_t s8{}, s8h{}; uint8_t u8{}, u8h{};
    int16_t s16{}, s16h{}; uint16_t u16{}, u16h{};
    int32_t s32{}, s32h{}; uint32_t u32{}, u32h{};
    int64_t s64{}, s64h{}; uint64_t u64{}, u64h{};
    float f32{}, f32h{}; double f64{}, f64h{};

    std::optional<std::vector<std::uint8_t>> bytearrayValue;
    std::optional<std::vector<std::uint8_t>> byteMask; // 0xFF=fixed, 0x00=wildcard
    std::string stringValue;

    MatchFlags flags = MatchFlags::EMPTY;

    template <typename T>
    static UserValue fromScalar(T value);

    template <typename T>
    static UserValue fromRange(T low, T high);
};
```

辅助：`flagForScalarType<T>() -> MatchFlags` 用于根据 T 自动选择 `B8/B16/B32/B64`。

## Mem64（当前值字节容器）

`utils.mem64` 提供轻量的字节容器，用于读/写目标进程的“当前值”字节：

```cpp
struct Mem64 {
    // 视图
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] const std::uint8_t* data() const noexcept;
    [[nodiscard]] std::uint8_t* data() noexcept;
    [[nodiscard]] std::span<const std::uint8_t> bytes() const noexcept;

    // 赋值
    void clear();
    void reserve(std::size_t n);
    void setBytes(const std::uint8_t* p, std::size_t n);
    void setBytes(std::span<const std::uint8_t> s);
    void setBytes(const std::vector<std::uint8_t>& v);
    void setString(const std::string& s);

    // 标量编解码（按主机端序）
    template <typename T> void setScalar(const T& v);
    template <typename T> std::optional<T> tryGet() const noexcept;
    template <typename T> T get() const; // 不足则抛出
};
```

## 示例

```cpp
// 创建 UserValue
auto uv = UserValue::fromScalar(std::int32_t{42});

// 保存旧值字节
Value old;
std::vector<std::uint8_t> pattern{0xDE,0xAD,0xBE,0xEF};
old.setBytes(pattern);
```

更多匹配与读取细节请参考 `scan/*` 与 `core/*` 中的具体实现。
