# 值类型模块文档

## 概述

`value` 模块为 NewScanmem 项目提供全面的值类型定义和匹配标志。它定义了各种数据类型表示、内存布局和实用结构，用于处理内存扫描操作中使用的不同数值类型、字节数组和通配符模式。

## 模块结构

```cpp
export module value;
```

## 依赖项

- `<array>` - 固定大小数组容器
- `<bit>` - 位操作
- `<cstdint>` - 固定宽度整数类型
- `<optional>` - 可选类型支持
- `<string>` - 字符串操作
- `<variant>` - 变体类型支持
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

### 2. Value 结构

支持多种数据类型的主要值容器。

```cpp
struct [[gnu::packed]] Value {
    std::variant<int8_t, uint8_t, int16_t, uint16_t, int32_t, uint32_t, int64_t,
                 uint64_t, float, double, std::array<uint8_t, sizeof(int64_t)>,
                 std::array<char, sizeof(int64_t)>> value;

    MatchFlags flags = MatchFlags::EMPTY;

    // 静态实用方法
    constexpr static void zero(Value& val);
};
```

#### 支持的数据类型

| C++ 类型 | MatchFlag | 描述 |
|----------|-----------|------|
| `int8_t` | `S8B` | 有符号 8 位整数 |
| `uint8_t` | `U8B` | 无符号 8 位整数 |
| `int16_t` | `S16B` | 有符号 16 位整数 |
| `uint16_t` | `U16B` | 无符号 16 位整数 |
| `int32_t` | `S32B` | 有符号 32 位整数 |
| `uint32_t` | `U32B` | 无符号 32 位整数 |
| `int64_t` | `S64B` | 有符号 64 位整数 |
| `uint64_t` | `U64B` | 无符号 64 位整数 |
| `float` | `F32B` | 32 位浮点 |
| `double` | `F64B` | 64 位浮点 |
| `std::array<uint8_t, 8>` | - | 原始字节数组 |
| `std::array<char, 8>` | - | 字符数组 |

#### 静态方法

##### zero(Value& val)

将 Value 对象清零。

**参数：**

- `val`: 要清零的 Value 对象

**效果：** 将 value 字段设置为默认值，flags 设置为 EMPTY

### 3. 内存布局

#### 字节序处理

Value 结构支持不同字节序的内存布局：

```cpp
// 小端序内存布局
struct LittleEndianLayout {
    uint8_t bytes[8];
};

// 大端序内存布局
struct BigEndianLayout {
    uint8_t bytes[8];
};
```

#### 对齐要求

- 结构体使用 `[[gnu::packed]]` 属性确保紧凑布局
- 支持跨平台内存对齐
- 兼容不同架构的内存模型

## 使用示例

### 基本使用

```cpp
import value;

// 创建整数类型的 Value
Value intValue;
intValue.value = static_cast<int32_t>(42);
intValue.flags = MatchFlags::S32B;

// 创建浮点类型的 Value
Value floatValue;
floatValue.value = 3.14159f;
floatValue.flags = MatchFlags::F32B;

// 创建字节数组类型的 Value
Value byteArrayValue;
std::array<uint8_t, 8> bytes = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
byteArrayValue.value = bytes;
```

### 类型检查

```cpp
// 检查 Value 的类型
if (std::holds_alternative<int32_t>(intValue.value)) {
    std::cout << "这是一个 32 位整数" << std::endl;
    int32_t val = std::get<int32_t>(intValue.value);
    std::cout << "值: " << val << std::endl;
}

// 检查标志
if (intValue.flags & MatchFlags::INTEGER) {
    std::cout << "这是一个整数类型" << std::endl;
}
```

### 类型转换

```cpp
// 从一种类型转换为另一种类型
Value sourceValue;
sourceValue.value = static_cast<uint16_t>(0x1234);
sourceValue.flags = MatchFlags::U16B;

// 转换为 32 位整数
Value targetValue;
targetValue.value = static_cast<uint32_t>(std::get<uint16_t>(sourceValue.value));
targetValue.flags = MatchFlags::U32B;
```

### 字节数组操作

```cpp
// 创建字节数组
std::array<uint8_t, 8> pattern = {0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x00, 0x00, 0x00};
Value patternValue;
patternValue.value = pattern;

// 访问字节
auto& bytes = std::get<std::array<uint8_t, 8>>(patternValue.value);
for (size_t i = 0; i < bytes.size(); ++i) {
    std::cout << "字节 " << i << ": 0x" << std::hex << (int)bytes[i] << std::endl;
}
```

## 匹配操作

### 精确匹配

```cpp
bool exactMatch(const Value& val1, const Value& val2) {
    // 检查类型兼容性
    if ((val1.flags & val2.flags) == MatchFlags::EMPTY) {
        return false;
    }
    
    // 执行类型特定的比较
    return std::visit([](const auto& v1, const auto& v2) {
        return v1 == v2;
    }, val1.value, val2.value);
}
```

### 范围匹配

```cpp
bool rangeMatch(const Value& val, const Value& min, const Value& max) {
    return std::visit([](const auto& v, const auto& min_val, const auto& max_val) {
        return v >= min_val && v <= max_val;
    }, val.value, min.value, max.value);
}
```

### 通配符匹配

```cpp
bool wildcardMatch(const Value& pattern, const Value& target) {
    // 检查是否为字节数组模式
    if (!std::holds_alternative<std::array<uint8_t, 8>>(pattern.value)) {
        return false;
    }
    
    auto& pattern_bytes = std::get<std::array<uint8_t, 8>>(pattern.value);
    auto& target_bytes = std::get<std::array<uint8_t, 8>>(target.value);
    
    for (size_t i = 0; i < pattern_bytes.size(); ++i) {
        if (pattern_bytes[i] != 0x00 && pattern_bytes[i] != target_bytes[i]) {
            return false;
        }
    }
    return true;
}
```

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

```cpp
// 安全的类型访问
std::optional<int32_t> getInt32Value(const Value& val) {
    if (std::holds_alternative<int32_t>(val.value)) {
        return std::get<int32_t>(val.value);
    }
    return std::nullopt;
}
```

### 范围错误

```cpp
// 检查值是否在有效范围内
bool isValidRange(const Value& val, MatchFlags expectedFlags) {
    if ((val.flags & expectedFlags) == MatchFlags::EMPTY) {
        return false;
    }
    
    return std::visit([](const auto& v) {
        // 执行范围检查
        return true; // 简化示例
    }, val.value);
}
```

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

## 最佳实践

### 类型安全

1. **始终检查类型**: 在访问值之前检查类型
2. **使用强类型**: 避免使用 void* 或原始指针
3. **验证标志**: 确保标志与值类型一致

### 内存管理

1. **避免拷贝**: 使用引用和移动语义
2. **及时清理**: 不再需要时及时释放资源
3. **预分配**: 预先分配足够的内存空间

### 性能优化

1. **批量操作**: 批量处理多个值以提高性能
2. **缓存结果**: 缓存频繁计算的结果
3. **使用 SIMD**: 对字节数组操作使用 SIMD 指令
