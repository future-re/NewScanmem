# Maps 模块文档

## 概述

Maps 模块提供读取和解析 Linux `/proc/[pid]/maps` 文件的功能，用于提取进程内存区域信息。这是一个现代化的 C++20 实现，用类型安全、符合 RAII 的接口替代了传统的 C 代码。

## 模块结构

```cpp
import maps;
```

## 核心组件

### 1. 区域类型

#### `region_type` 枚举

表示内存区域的分类：

```cpp
enum class RegionType : uint8_t {
    MISC,   // 杂项内存区域
    EXE,    // 可执行文件二进制区域
    CODE,   // 代码段（共享库等）
    HEAP,   // 堆内存区域
    STACK   // 栈内存区域
};

constexpr std::array<std::string_view, 5> REGION_TYPE_NAMES = {
    "misc", "exe", "code", "heap", "stack"
};
```

### 2. 扫描级别

#### `RegionScanLevel` 枚举

控制包含哪些内存区域的扫描：

```cpp
enum class RegionScanLevel : uint8_t {
    ALL,                       // 所有可读区域
    ALL_RW,                    // 所有可读/可写区域
    HEAP_STACK_EXECUTABLE,     // 堆、栈和可执行区域
    HEAP_STACK_EXECUTABLE_BSS  // 上述加上 BSS 段
};
```

### 3. 区域元数据

#### `RegionFlags` 结构

包含内存区域的权限和状态标志：

```cpp
struct RegionFlags {
    bool read : 1;    // 读权限
    bool write : 1;   // 写权限
    bool exec : 1;    // 执行权限
    bool shared : 1;  // 共享映射
    bool private_ : 1; // 私有映射
};
```

#### `Region` 结构

关于内存区域的完整信息：

```cpp
struct Region {
    void* start;           // 起始地址
    std::size_t size;      // 区域大小（字节）
    RegionType type;       // 区域分类
    RegionFlags flags;     // 权限标志
    void* loadAddr;        // ELF 文件的加载地址
    std::string filename;  // 关联文件路径
    std::size_t id;        // 唯一标识符
    
    // 辅助方法
    [[nodiscard]] bool isReadable() const noexcept;
    [[nodiscard]] bool isWritable() const noexcept;
    [[nodiscard]] bool isExecutable() const noexcept;
    [[nodiscard]] bool isShared() const noexcept;
    [[nodiscard]] bool isPrivate() const noexcept;
    
    [[nodiscard]] std::pair<void*, std::size_t> asSpan() const noexcept;
    [[nodiscard]] bool contains(void* address) const noexcept;
};
```

## 使用示例

### 基本用法

```cpp
import maps;

// 读取进程的所有内存区域
auto result = maps::readProcessMaps(1234);
if (result) {
    for (const auto& region : *result) {
        std::cout << std::format("区域: {}-{} ({})\n", 
                               region.start, 
                               static_cast<char*>(region.start) + region.size,
                               REGION_TYPE_NAMES[static_cast<size_t>(region.type)]);
    }
}
```

### 过滤扫描

```cpp
// 只扫描堆和栈区域
auto regions = maps::readProcessMaps(
    pid, 
    maps::RegionScanLevel::HEAP_STACK_EXECUTABLE
);

if (regions) {
    for (const auto& region : *regions) {
        if (region.type == maps::region_type::heap) {
            std::cout << "找到堆区域: " << region.filename << "\n";
        }
    }
}
```

### 错误处理

```cpp
auto result = maps::read_process_maps(pid);
if (!result) {
    std::cerr << "错误: " << result.error().message << "\n";
    return;
}
```

## 类：`maps_reader`

### 静态方法

#### `read_process_maps`

从进程读取内存区域：

```cpp
[[nodiscard]] static std::expected<std::vector<region>, error> 
read_process_maps(pid_t pid, region_scan_level level = region_scan_level::all);
```

**参数：**

- `pid`: 目标进程 ID
- `level`: 扫描级别过滤器（默认：`all`）

**返回值：**

- `std::expected` 包含区域向量或错误信息

**错误处理：**

- 返回 `std::error_code` 和适当的错误消息
- 常见错误：文件未找到、权限被拒绝、格式无效

## 错误处理模块

### `maps_reader::error` 结构

```cpp
struct error {
    std::string message;   // 人类可读的错误描述
    std::error_code code;  // 系统错误代码
};
```

### 常见错误场景

1. **进程不存在**：`no_such_file_or_directory`
2. **权限被拒绝**：`permission_denied`
3. **格式无效**：`invalid_argument`

## 高级功能

### 区域分析

```cpp
// 检查地址是否在任意区域内
auto regions = maps::read_process_maps(pid);
void* address = /* 某个地址 */;

for (const auto& region : *regions) {
    if (region.contains(address)) {
        std::cout << "地址在: " << region.filename << "中找到\n";
        break;
    }
}
```

### 权限检查

```cpp
// 查找可写可执行区域（潜在的 shellcode 目标）
for (const auto& region : *regions) {
    if (region.isWritable() && region.isExecutable()) {
        std::cout << "WX 区域: " << region.filename << "\n";
    }
}
```

## 性能说明

- **内存高效**：对小文件名使用带有 SSO 的 `std::string`
- **零拷贝**：直接从映射行提取字符串
- **提前过滤**：根据扫描级别在解析期间过滤区域
- **RAII**：通过 `std::ifstream` 自动清理资源

## 线程安全

- **线程安全**：所有方法对并发访问都是线程安全的
- **无共享状态**：每次调用创建独立状态
- **不可变结果**：返回的向量包含不可变数据

## 平台兼容性

- **仅 Linux**：需要 `/proc/[pid]/maps` 文件系统
- **需要 C++23**：使用 `std::expected` 和其他 C++23 特性
- **支持 64 位**：处理 32 位和 64 位地址空间

## 从传统 C 代码迁移

### 传统 C → 现代 C++

| 传统 C | 现代 C++ |
|--------|----------|
| `region_t*` | `maps::region` |
| `list_t` | `std::vector<region>` |
| `bool return` | `std::expected` |
| `char*` 文件名 | `std::string` |
| 手动内存管理 | RAII |
| 错误代码 | 异常安全的错误处理 |

## 示例

### 完整工作示例

```cpp
#include <iostream>
import maps;

int main() {
    pid_t target_pid = 1234; // 替换为实际 PID
    
    auto regions = maps::readProcessMaps(target_pid);
    if (!regions) {
        std::cerr << "读取映射失败: " << regions.error().message << "\n";
        return 1;
    }
    
    std::cout << "找到 " << regions->size() << " 个内存区域:\n";
    
    for (const auto& region : *regions) {
        std::cout << std::format(
            "0x{:x}-0x{:x} {} {} {}\n",
            reinterpret_cast<uintptr_t>(region.start),
            reinterpret_cast<uintptr_t>(region.start) + region.size,
            region.isReadable() ? 'r' : '-',
            region.isWritable() ? 'w' : '-',
            region.isExecutable() ? 'x' : '-',
            region.filename.empty() ? "[匿名]" : region.filename
        );
    }
    
    return 0;
}
```
