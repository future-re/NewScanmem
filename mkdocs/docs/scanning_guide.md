# 指南：使用 Scanner 进行内存扫描

`Scanner` 类是 `NewScanmem` 中执行内存扫描的核心接口。本指南将引导您了解如何使用它来查找和过滤目标进程中的数据。

## 核心概念

要成功进行扫描，您需要了解以下几个核心概念：

### 1. `Scanner` 类

这是您进行所有扫描操作的入口点。首先，您需要用目标进程的 `PID` 来实例化它。

```cpp
#include "core/scanner.cppm"

pid_t target_pid = 12345;
Scanner scanner(target_pid);
```

### 2. `ScanOptions`：扫描的“控制面板”

这是一个结构体，用于精确配置您想如何进行扫描。最重要的字段包括：

*   `dataType`: 您想扫描什么类型的数据。
*   `matchType`: 您想如何比较这些数据。
*   `step`: 扫描时每次移动多少字节。

### 3. `ScanDataType`：扫描什么？

这个枚举决定了扫描的目标数据类型和大小。

| `ScanDataType` | 大小 | 描述 |
| :--- | :--- | :--- |
| `INTEGER8` | 1 字节 | 8位整数 |
| `INTEGER16` | 2 字节 | 16位整数 |
| `INTEGER32` | 4 字节 | 32位整数 |
| `INTEGER64` | 8 字节 | 64位整数 |
| `FLOAT32` | 4 字节 | 32位浮点数 |
| `FLOAT64` | 8 字节 | 64位浮点数 |
| `STRING` | 可变 | C风格字符串 |
| `BYTEARRAY`| 可变 | 字节数组 |
| `ANYINTEGER`| 可变 | 任意整数类型 |

### 4. `ScanMatchType`：如何比较？

这个枚举定义了比较内存数据的方式。我们可以将其分为两大类：

#### A. 用于“初次扫描”

这类扫描通常需要您提供一个具体的值。

| `ScanMatchType` | 描述 |
| :--- | :--- |
| `MATCHEQUALTO` | 等于您提供的值 |
| `MATCHNOTEQUALTO` | 不等于您提供的值 |
| `MATCHGREATERTHAN`| 大于您提供的值 |
| `MATCHLESSTHAN` | 小于您提供的值 |

#### B. 用于“后续过滤扫描”

这类扫描会利用前一次扫描的快照（`oldValue`），比较数值的变化情况。

> **[!] 注意**
>
> 这是API的设计目标。目前底层引擎的过滤功能尚在开发中（标记为`TODO`），暂不可用。

| `ScanMatchType` | 描述 |
| :--- | :--- |
| `MATCHCHANGED` | 值已改变 |
| `MATCHNOTCHANGED` | 值未改变 |
| `MATCHINCREASED` | 值已增加 |
| `MATCHDECREASED` | 值已减少 |
| `MATCHINCREASEDBY`| 值增加了特定数值 |
| `MATCHDECREASEDBY`| 值减少了特定数值 |

### 5. `step`：扫描步长（对齐方式）

`step` 参数控制扫描器在内存中移动的距离。

*   **`step = 1` (默认):** 非对齐扫描。扫描器在每个字节处都进行检查。最彻底，也最慢。
*   **`step = 4` (对于4字节数据):** 对齐扫描。扫描器只在4字节对齐的地址上检查。速度快，但可能错过未对齐的数据。

---

## 工作流程与示例

### 场景1：初次扫描

假设我们想在一个进程中查找所有值为 `100` 的4字节整数。

```cpp
#include "core/scanner.cppm"
#include "value/scalar.cppm" // for UserValue

// 1. 创建 Scanner 实例
pid_t target_pid = 12345;
Scanner scanner(target_pid);

// 2. 配置扫描选项
// 使用辅助函数可以更方便地创建选项
auto opts = makeNumericScanOptions(ScanDataType::INTEGER32, ScanMatchType::MATCHEQUALTO);
opts.step = 4; // 假设我们知道数值是4字节对齐的，以加快速度

// 3. 准备要搜索的值
UserValue value_to_find = UserValue::from<int32_t>(100);

// 4. 执行扫描
auto result = scanner.performScan(opts, &value_to_find);

// 5. 检查结果
if (result) {
    auto stats = *result;
    std::cout << "扫描完成！" << std::endl;
    std::cout << "访问的区域数: " << stats.regionsVisited << std::endl;
    std::cout << "扫描的总字节数: " << stats.bytesScanned << std::endl;
    std::cout << "找到的匹配项数: " << scanner.getMatchCount() << std::endl;
} else {
    std::cerr << "扫描失败: " << result.error() << std::endl;
}
```

### 场景2：后续过滤扫描（未来功能）

假设我们已经找到了所有值为 `100` 的地址。现在，我们在游戏中让角色的生命值减少，并想找出哪个地址的值发生了变化。

> **[!] 警告**
>
> 以下为API设计功能的演示，当前版本尚不支持。

```cpp
// (续上文... 假设初次扫描已完成)

// 1. 配置过滤扫描选项 (查找值已改变的地址)
auto filter_opts = makeNumericScanOptions(ScanDataType::INTEGER32, ScanMatchType::MATCHCHANGED);

// 2. 再次执行扫描，这次不需要提供具体数值
// 扫描器内部会使用上一次的快照进行比较
auto filter_result = scanner.performScan(filter_opts);

// 3. 检查结果
if (filter_result) {
    std::cout << "过滤完成！" << std::endl;
    std::cout << "剩余匹配项数: " << scanner.getMatchCount() << std::endl;
    // 此时 scanner.getMatches() 返回的就是那些值从100变为其他值的地址列表
}
```
