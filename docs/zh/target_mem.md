# 目标内存模块文档

## 概述

`targetmem` 模块为 NewScanmem 项目提供内存匹配和分析结构。它包括用于管理内存匹配、存储旧值以及执行内存区域操作的类，支持高效的搜索和操作。

## 模块结构

```cpp
export module targetmem;
```

## 依赖项

- `<algorithm>` - 标准算法
- `<cassert>` - 断言宏
- `<cctype>` - 字符分类
- `<cstddef>` - 大小类型定义
- `<cstdint>` - 固定宽度整数类型
- `<cstdio>` - C 标准 I/O
- `<cstdlib>` - C 标准库
- `<cstring>` - C 字符串操作
- `<optional>` - 可选类型支持
- `<sstream>` - 字符串流操作
- `<string>` - 字符串操作
- `<vector>` - 动态数组容器
- `value` 模块 - 值类型定义
- `show_message` 模块 - 消息打印系统

## 核心功能

### 1. 内存匹配标志

使用来自 `value` 模块的 `MatchFlags` 来指示内存匹配的类型和状态。

### 2. OldValueAndMatchInfo 结构

```cpp
struct OldValueAndMatchInfo {
    uint8_t old_value;      // 原始字节值
    MatchFlags match_info;  // 匹配类型和状态标志
};
```

### 3. MatchesAndOldValuesSwath 类

表示具有匹配信息的连续内存区域。

```cpp
class MatchesAndOldValuesSwath {
public:
    void* firstByteInChild = nullptr;                    // 起始地址
    std::vector<OldValueAndMatchInfo> data;              // 匹配数据

    // 构造函数
    MatchesAndOldValuesSwath() = default;

    // 元素管理
    void addElement(void* addr, uint8_t byte, MatchFlags matchFlags);

    // 字符串表示工具
    std::string toPrintableString(size_t idx, size_t len) const;
    std::string toByteArrayText(size_t idx, size_t len) const;
};
```

#### 方法

##### addElement(void* addr, uint8_t byte, MatchFlags matchFlags)

向 swath 添加新的内存匹配。

**参数：**

- `addr`: 匹配的内存地址
- `byte`: 此地址的字节值
- `matchFlags`: 匹配类型和标志

##### toPrintableString(size_t idx, size_t len)

将内存字节转换为可打印的 ASCII 字符串。

**参数：**

- `idx`: 数据向量中的起始索引
- `len`: 要转换的字节数

**返回：** 可打印字符串，不可打印字符显示为 '.'

##### toByteArrayText(size_t idx, size_t len)

将内存字节转换为十六进制文本表示。

**参数：**

- `idx`: 数据向量中的起始索引
- `len`: 要转换的字节数

**返回：** 空格分隔的十六进制值

### 4. MatchesAndOldValuesArray 类

管理多个 swath 并提供搜索操作。

```cpp
class MatchesAndOldValuesArray {
public:
    std::vector<MatchesAndOldValuesSwath> swaths;        // 内存区域集合

    // 构造函数
    MatchesAndOldValuesArray() = default;

    // 搜索操作
    std::optional<size_t> findSwathIndex(void* addr) const;
    std::optional<size_t> findElementIndex(void* addr) const;

    // 数据访问
    const OldValueAndMatchInfo* getElement(void* addr) const;
    OldValueAndMatchInfo* getElement(void* addr);

    // 内存管理
    void clear();
    size_t size() const;
};
```

#### MatchesAndOldValuesArray方法

##### findSwathIndex(void* addr) const

查找包含指定地址的 swath 索引。

**参数：**

- `addr`: 要查找的内存地址

**返回：** 包含该地址的 swath 索引，如果未找到则返回 std::nullopt

##### findElementIndex(void* addr) const

查找指定地址在 swath 中的元素索引。

**参数：**

- `addr`: 要查找的内存地址

**返回：** 元素在 swath 中的索引，如果未找到则返回 std::nullopt

##### getElement(void* addr)

获取指定地址的元素数据。

**参数：**

- `addr`: 内存地址

**返回：** 指向 OldValueAndMatchInfo 的指针，如果未找到则返回 nullptr

## 使用示例

### 基本使用

```cpp
import targetmem;

// 创建内存匹配数组
MatchesAndOldValuesArray matches;

// 添加内存匹配
void* addr1 = (void*)0x1000;
void* addr2 = (void*)0x1001;

MatchesAndOldValuesSwath swath;
swath.addElement(addr1, 0x42, MatchFlags::EXACT_MATCH);
swath.addElement(addr2, 0x7F, MatchFlags::GREATER_THAN);

matches.swaths.push_back(swath);
```

### 搜索操作

```cpp
// 查找包含地址的 swath
auto swathIndex = matches.findSwathIndex(addr1);
if (swathIndex) {
    std::cout << "找到 swath 索引: " << *swathIndex << std::endl;
}

// 查找元素索引
auto elementIndex = matches.findElementIndex(addr1);
if (elementIndex) {
    std::cout << "找到元素索引: " << *elementIndex << std::endl;
}

// 获取元素数据
const auto* element = matches.getElement(addr1);
if (element) {
    std::cout << "旧值: 0x" << std::hex << (int)element->old_value << std::endl;
}
```

### 字符串表示

```cpp
// 转换为可打印字符串
std::string printable = swath.toPrintableString(0, 10);
std::cout << "可打印字符串: " << printable << std::endl;

// 转换为字节数组文本
std::string byteArray = swath.toByteArrayText(0, 10);
std::cout << "字节数组: " << byteArray << std::endl;
```

## 内存管理

### 内存布局

每个 `MatchesAndOldValuesSwath` 表示一个连续的内存区域：

```text
+----------------+----------------+----------------+
| 地址 0x1000    | 地址 0x1001    | 地址 0x1002    |
| 旧值: 0x42     | 旧值: 0x7F     | 旧值: 0xAA     |
| 标志: EXACT    | 标志: GREATER  | 标志: LESS     |
+----------------+----------------+----------------+
```

### 内存对齐

- 地址按字节对齐
- 数据结构紧凑存储
- 支持跨平台内存布局

### 内存安全

- 边界检查防止越界访问
- 空指针检查
- 异常安全保证

## 性能优化

### 搜索优化

1. **二分搜索**: 在有序 swath 中使用二分搜索
2. **地址范围检查**: 快速过滤不相关的 swath
3. **缓存友好**: 连续内存访问模式

### 内存使用

1. **紧凑存储**: 最小化内存占用
2. **动态增长**: 按需分配内存
3. **内存池**: 减少分配开销

### 算法复杂度

- **查找 swath**: O(log n) 其中 n 是 swath 数量
- **查找元素**: O(log m) 其中 m 是 swath 中的元素数量
- **添加元素**: O(1) 平均情况

## 错误处理

### 常见错误

1. **无效地址**: 空指针或无效内存地址
2. **越界访问**: 访问超出范围的索引
3. **内存不足**: 分配失败

### 错误处理策略

- 使用 std::optional 表示查找失败
- 返回 nullptr 表示无效访问
- 抛出异常处理严重错误

## 扩展性

### 自定义匹配类型

```cpp
enum class CustomMatchFlags : uint8_t {
    CUSTOM_MATCH_1,
    CUSTOM_MATCH_2,
    // ... 其他类型
};
```

### 自定义数据结构

```cpp
struct CustomMatchInfo {
    uint8_t old_value;
    CustomMatchFlags flags;
    std::string additional_info;
};
```

### 插件支持

```cpp
class MatchPlugin {
public:
    virtual bool processMatch(void* addr, uint8_t value) = 0;
    virtual std::string getDescription() const = 0;
};
```
