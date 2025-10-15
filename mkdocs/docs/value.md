# 值类型模块文档

## 概述

`value` 模块为 NewScanmem 项目提供全面的值类型定义和匹配标志。它定义了各种数据类型表示、内存布局和实用结构，用于处理内存扫描操作中使用的不同数值类型、字节数组和通配符模式。

## 模块结构

```cpp
export module value;
```

## 依赖项

- `<cstdint>` - 固定宽度整数类型
- `<cstring>` - 字节拷贝
- `<optional>` - 可选类型支持
- `<span>` - 只读/可写字节视图
- `<string>` - 字符串操作
- `<type_traits>` - 类型工具
- `<vector>` - 动态数组容器

## 核心功能

### 1. 匹配标志枚举

```cpp
enum class [[gnu::packed]] MatchFlags : uint16_t {
    EMPTY = 0,

    // 基本数值类型
    U8B = 1 << 0,   // 无符号 8 位
    S8B = 1 << 1,   // 有符号 8 位
    U16B = 1 << 2,  // 无符号 16 位
    S16B = 1 << 3,  // 有符号 16 位
    U32B = 1 << 4,  // 无符号 32 位
    S32B = 1 << 5,  // 有符号 32 位
    U64B = 1 << 6,  // 无符号 64 位
    S64B = 1 << 7,  // 有符号 64 位

    // 浮点类型
    F32B = 1 << 8,  // 32 位浮点
    F64B = 1 << 9,  // 64 位浮点

    // 复合类型
    I8B = U8B | S8B,      // 任意 8 位整数
    I16B = U16B | S16B,   // 任意 16 位整数
    I32B = U32B | S32B,   // 任意 32 位整数
    I64B = U64B | S64B,   // 任意 64 位整数

    INTEGER = I8B | I16B | I32B | I64B,  // 所有整数类型
    FLOAT = F32B | F64B,                  // 所有浮点类型
    ALL = INTEGER | FLOAT,                // 所有支持的类型

    // 基于字节的分组
    B8 = I8B,           // 8 位块
    B16 = I16B,         // 16 位块
    B32 = I32B | F32B,  // 32 位块
    B64 = I64B | F64B,  // 64 位块

    MAX = 0xffffU  // 最大标志值
};
```

### 2. Value 结构（统一为字节存储）

Value 作为“历史值（旧值）”容器，底层仅存储连续字节，类型/宽度语义由 `flags` 表示。

```cpp
struct [[gnu::packed]] Value {
    std::vector<uint8_t> bytes;    // 历史值字节
    MatchFlags flags = MatchFlags::EMPTY; // 类型/宽度标志（严格数值路径需要）

    // 复位为零状态
    constexpr static void zero(Value& val);

    // 获取只读字节视图
    std::span<const uint8_t> view() const noexcept;

    // 按字节设置（可附带 flag）
    void setBytes(const uint8_t* data, std::size_t len);
    void setBytes(const std::vector<uint8_t>& val);
    void setBytesWithFlag(const uint8_t* data, std::size_t len, MatchFlags f);
    void setBytesWithFlag(const std::vector<uint8_t>& val, MatchFlags f);

    // 按标量设置（拷贝其内存表示）；Typed 版本自动设置正确 flag
    template <typename T> void setScalar(const T& v);
    template <typename T> void setScalarWithFlag(const T& v, MatchFlags f);
    template <typename T> void setScalarTyped(const T& v);
};
```

要点：
- 数值比较路径“严格校验”：只有当 `flags` 与期望类型相符且 `bytes.size() >= sizeof(T)` 时，旧值才会被解码参与比较。
- 字节串/字符串匹配不依赖 `flags`，不受严格策略限制。

### 3. Mem64 结构（当前值读取缓冲）

Mem64 表示“当前位置读取到的字节”。

```cpp
struct [[gnu::packed]] Mem64 {
    std::vector<uint8_t> buffer;   // 当前值字节

    // 读取/写入
    template <typename T> T get() const;  // 用 memcpy 解码 T
    std::span<const uint8_t> bytes() const noexcept; // 只读视图
    void setBytes(const uint8_t* data, std::size_t len);
    void setBytes(const std::vector<uint8_t>& data);
    void setString(const std::string& s);
    template <typename T> void setScalar(const T& v);
};
```

端序处理：对于数值类型，通过 `swapIfReverse`（在扫描模块内）或对 `Value` 使用 `fixEndianness` 将旧值与当前值端序对齐。

## 使用示例

### 基本使用（数值严格 + 字节自由）

```cpp
import value;

// 保存旧值（数值，自动设置正确 flag）
Value intValue;
intValue.setScalarTyped<int32_t>(42);

// 创建浮点类型的 Value
Value floatValue;
floatValue.setScalarTyped<float>(3.14159f);

// 保存旧值（原始字节，不设 flag 也可）
Value byteArrayValue;
std::vector<uint8_t> bytes = {0x01,0x02,0x03,0x04};
byteArrayValue.setBytes(bytes);
```

### 类型/标志检查

```cpp
// 检查标志
if (intValue.flags & MatchFlags::INTEGER) {
    std::cout << "这是一个整数类型" << std::endl;
}
```

### 字节数组操作

```cpp
Value patternValue;
std::vector<uint8_t> pattern = {0xDE,0xAD,0xBE,0xEF};
patternValue.setBytes(pattern);
for (auto b : patternValue.view()) { /* 遍历字节 */ }
```

## 匹配操作

匹配逻辑在 `scanroutines` 模块中实现：
- 数值：严格按照 `flags` 与类型匹配（解码旧值时要求类型一致）。
- 字节串/字符串：滑动搜索；可选掩码（0xFF=精确，0x00=通配）；字符串支持 Boost.Regex。

更多匹配示例（范围、掩码、正则）请参考 `scanroutines` 模块文档与实现。

## 性能优化

### 内存布局优化

1. **紧凑存储**: 使用 packed 属性减少内存占用
2. **缓存友好**: 连续内存访问模式
3. **对齐优化**: 针对目标架构优化对齐

### 类型检查优化

1. **编译时检查**: 使用 static_assert 进行编译时验证
2. **运行时优化**: 高效的类型检查算法
3. **缓存结果**: 缓存频繁的类型检查结果

### 转换优化

1. **零拷贝**: 尽可能避免数据拷贝
2. **内联函数**: 使用内联函数减少函数调用开销
3. **SIMD 优化**: 对字节数组操作使用 SIMD 指令

## 错误处理

### 类型错误

数值路径下，旧值解码时要求 `flags` 与期望类型一致且长度足够，否则上层应当放弃解码。

### 范围错误

上层应根据 `flags` 推断宽度并进行边界检查，`Value` 本身仅提供字节视图与标志，不直接进行范围判断。

## 扩展性

### 自定义类型

```cpp
// 添加自定义数据类型
enum class CustomMatchFlags : uint16_t {
    CUSTOM_TYPE_1 = 1 << 10,
    CUSTOM_TYPE_2 = 1 << 11,
};

// 扩展 Value 结构
struct ExtendedValue : public Value {
    CustomMatchFlags customFlags = CustomMatchFlags::CUSTOM_TYPE_1;
    std::string additionalData;
};
```

### 插件系统

```cpp
// 值处理插件接口
class ValueProcessor {
public:
    virtual bool process(Value& val) = 0;
    virtual std::string getDescription() const = 0;
};

// 具体实现
class CustomValueProcessor : public ValueProcessor {
public:
    bool process(Value& val) override {
        // 自定义处理逻辑
        return true;
    }
    
    std::string getDescription() const override {
        return "自定义值处理器";
    }
};
```
