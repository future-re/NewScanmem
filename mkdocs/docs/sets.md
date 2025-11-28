# 集合模块文档

## 概述

`sets` 模块为 NewScanmem 项目提供集合操作和解析工具。它包括一个用于管理整数集合的 `Set` 类，以及一个强大的集合表达式解析器，支持范围、十六进制/十进制数字和取反操作。

## 模块结构

```cpp
export module sets;
```

## 依赖项

- `<algorithm>` - 标准算法
- `<boost/regex.hpp>` - 正则表达式支持
- `<boost/spirit/include/phoenix.hpp>` - Boost Spirit Phoenix 语义动作
- `<boost/spirit/include/qi.hpp>` - Boost Spirit Qi 解析
- `<cctype>` - 字符分类
- `<compare>` - 三路比较
- `<cstdlib>` - C 标准库
- `<stdexcept>` - 标准异常
- `<string>` - 字符串操作
- `<vector>` - 动态数组容器

## 核心功能

### 1. Set 结构

```cpp
export struct Set {
    std::vector<size_t> buf;
    
    size_t size() const;
    void clear();
    static int cmp(const size_t& i1, const size_t& i2);
};
```

#### 方法

- **size()**: 返回集合中的元素数量
- **clear()**: 移除集合中的所有元素
- **cmp()**: 使用三路比较对两个 size_t 值进行静态比较函数

### 2. 集合表达式解析器

```cpp
export bool parse_uintset(std::string_view lptr, Set& set, size_t maxSZ);
```

#### 支持的表达式格式

- **单个数字**: `42`, `0x2A`
- **范围**: `10..20`, `0x10..0xFF`
- **多个值**: `1,2,3,4,5`
- **混合格式**: `1,5,10..15,0x20`
- **取反**: `!1,2,3` (除了 1,2,3 之外的所有数字)
- **十六进制**: `0x10`, `0xFF`, `0xdeadbeef`

#### 参数

- **lptr**: 要解析的集合表达式字符串
- **set**: 用于存储结果的 Set 对象
- **maxSZ**: 最大允许值（独占上界）

#### 返回值

- `true`: 解析成功
- `false`: 解析失败（语法无效、超出范围等）

### 3. 已弃用的内存管理

```cpp
[[deprecated("This interface is deprecated...")]]
constexpr auto inc_arr_sz = [](size_t** valarr, size_t* arr_maxsz, size_t maxsz) -> bool;
```

一个已弃用的 C 风格内存管理工具，用于动态数组大小调整。

## 使用示例

### 基本集合解析

```cpp
import sets;

Set mySet;
bool success = parse_uintset("1,2,3,4,5", mySet, 100);
if (success) {
    std::cout << "集合包含 " << mySet.size() << " 个元素\n";
}
```

### 范围解析

```cpp
Set rangeSet;
parse_uintset("10..20", rangeSet, 100);
// 结果: {10, 11, 12, ..., 20}
```

### 十六进制解析

```cpp
Set hexSet;
parse_uintset("0x10..0x20", hexSet, 100);
// 结果: {16, 17, 18, ..., 32}
```

### 混合格式解析

```cpp
Set mixedSet;
parse_uintset("1,5,10..15,0x20", mixedSet, 100);
// 结果: {1, 5, 10, 11, 12, 13, 14, 15, 32}
```

### 取反操作示例

```cpp
Set invertedSet;
parse_uintset("!1,2,3", invertedSet, 10);
// 结果: {0, 4, 5, 6, 7, 8, 9} (除了 1,2,3 之外的所有数字)
```

## 表达式语法

### 数字格式

- **十进制**: `123`, `456`, `789`
- **十六进制**: `0x7B`, `0x1C8`, `0x315`
- **混合**: 在同一表达式中可以混合使用

### 范围语法

- **基本范围**: `start..end`
- **十六进制范围**: `0xstart..0xend`
- **包含边界**: 范围包含起始值和结束值

### 分隔符

- **逗号分隔**: `1,2,3,4,5`
- **空格分隔**: `1 2 3 4 5` (在某些情况下)
- **混合分隔**: `1, 2, 3, 4, 5`

### 取反操作

- **语法**: `!expression`
- **效果**: 包含指定范围内除了表达式指定值之外的所有数字
- **示例**: `!1,2,3` 在范围 0..10 中表示 `{0,4,5,6,7,8,9,10}`