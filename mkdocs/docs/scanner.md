# 扫描器（core.scanner）——精确 API 文档

下面文档直接基于 `src/core/scanner.cppm` 导出的接口，逐项列出函数/方法签名、参数语义、返回值、行为、副作用以及示例代码，便于直接在代码中调用与测试。

## 必要导入

```cpp
import core.scanner;   // Scanner 及便捷函数
import scan.types;     // ScanDataType / ScanMatchType
import scan.engine;    // ScanOptions / ScanStats
import core.targetmem; // MatchesAndOldValuesArray
import value;          // UserValue
```

---

## 主要类型与结构（快速索引）

- Scanner：高层扫描会话对象，包装底层 `scan.engine`。
- ScanOptions：扫描配置（数据类型、匹配方式、步长、块大小、区域级别等），定义在 `scan.engine`。
- ScanStats：扫描统计信息（regionsVisited、bytesScanned、matches）。
- ScanDataType：在 `scan.types` 中定义的数据类型枚举（如 `INTEGER32/INTEGER64/FLOAT32/FLOAT64/BYTEARRAY/STRING` 等）。
- ScanMatchType：在 `scan.types` 中定义的匹配方式枚举（如 `MATCHEQUALTO/MATCHCHANGED/MATCHRANGE` 等）。
- MatchesAndOldValuesArray：扫描结果容器，由若干 `MatchesAndOldValuesSwath` 组成，每个 swath 包含：
	- `void* firstByteInChild`：该 swath 在目标进程中的第一个字节地址。
	- `std::vector<OldValueAndMatchInfo> data`：每个元素对应目标内存的 1 字节；`OldValueAndMatchInfo` 包含 `oldValue`(uint8_t) 与 `matchInfo`(MatchFlags)。
	- `rangeMarks`：按区间标记的辅助信息（UI/后处理用）。

---

## 类：Scanner（接口与行为）

签名摘要：

```cpp
export class Scanner {
public:
	explicit Scanner(pid_t pid);

	[[nodiscard]] auto performScan(const ScanOptions& opts,
																 const UserValue* value = nullptr)
			-> std::expected<ScanStats, std::string>;

	[[nodiscard]] auto getMatches() const -> const MatchesAndOldValuesArray&;
	[[nodiscard]] auto getMatchesMut() -> MatchesAndOldValuesArray&;
	[[nodiscard]] auto getLastStats() const -> const ScanStats&;

	auto clearMatches() -> void;
	[[nodiscard]] auto getMatchCount() const -> std::size_t;
	[[nodiscard]] auto hasMatches() const -> bool;
	[[nodiscard]] auto getPid() const -> pid_t;
};
```

逐项说明：

- Scanner(pid_t pid)
	- 说明：创建一个绑定到 `pid` 的扫描器实例。该构造函数不会自动附加 ptrace 或修改目标进程；仅用于后续 `performScan` 的上下文。

- performScan(const ScanOptions& opts, const UserValue* value = nullptr)
	- 说明：启动一次完整扫描。
	- 参数：
		- `opts`：扫描配置，关键字段包括：
			- `dataType`（ScanDataType）：如 `INTEGER32`、`FLOAT32`、`STRING` 等；
			- `matchType`（ScanMatchType）：匹配策略（如 `MATCHEQUALTO`）；
			- `step`：扫描步长（以字节为单位）；
			- `blockSize`：每次读取的内存块大小；
			- `regionLevel`：来自 `core.maps`，用于限制扫描到哪些内存区域。
		- `value`：当 `matchType` 需要用户值（例如 `MATCHEQUALTO/MATCHRANGE`）时传入，指向 `UserValue`。
	- 返回：`std::expected<ScanStats, std::string>`
		- 成功（has_value）：返回 `ScanStats` 并且内部 `m_matches` 被更新为本次扫描结果；用户可调用 `getMatches()` 读取。
		- 失败（has_error）：返回错误字符串，例如权限或 I/O 相关错误。
	- 副作用：内部 `m_matches` 被覆盖为新的扫描结果；`m_lastStats` 被更新。

- getMatches() / getMatchesMut()
	- 说明：获取上一次扫描后的匹配结果。返回的 `MatchesAndOldValuesArray` 包含若干 swath，每个 swath 的 `data` 中的元素对应目标进程中连续的字节序列。
	- 使用注意：
		- 若要修改匹配标记或追加历史值，请使用 `getMatchesMut()` 获得可变引用。
		- 访问具体匹配地址时：`addr = (void*)((uintptr_t)swath.firstByteInChild + index)`。

- getLastStats()
	- 说明：返回 `ScanStats`，包含 `regionsVisited`、`bytesScanned`、`matches`（统计值）。

- clearMatches()
	- 说明：清空内部匹配结果（等同于 `m_matches.swaths.clear()`）。

- getMatchCount()
	- 说明：遍历 `m_matches` 并统计 `matchInfo != MatchFlags::EMPTY` 的字节数量，返回该数值。

- hasMatches()
	- 说明：便捷判断，等价于 `getMatchCount() > 0`。

- getPid()
	- 说明：返回构造时绑定的 `pid`。

---

## 便捷函数（在 `core.scanner` 中导出）

- `getDefaultScanOptions()`
	- 返回：一个填充了默认值的 `ScanOptions`（等价于 `ScanOptions{}`）。

- `makeNumericScanOptions(ScanDataType dataType, ScanMatchType matchType = ScanMatchType::MATCHEQUALTO)`
	- 用途：快速构建用于数值（整数/浮点）扫描的 `ScanOptions`。
	- 建议：对整数查找通常使用 `ScanDataType::INTEGER32` 或 `INTEGER64`，并使用 `MATCHEQUALTO` 作为 matchType 来查找确切值。

- `makeStringScanOptions(ScanMatchType matchType = ScanMatchType::MATCHEQUALTO)`
	- 用途：构建字符串搜索的 `ScanOptions`；函数会把 `dataType` 设为 `ScanDataType::STRING` 并把 `step` 设为 1（逐字节检查）。

---

## 精确示例：查找 int32 值并解析地址（推荐代码片段）

下列示例逐步展示如何使用 `Scanner`：

```cpp
import core.scanner;
import scan.engine;   // ScanOptions / ScanStats
import scan.types;    // ScanDataType / ScanMatchType
import value;         // UserValue
import core.targetmem;// MatchesAndOldValuesArray

pid_t targetPid = /* child pid */;
Scanner scanner(targetPid);

// 1) 配置扫描选项：32-bit 整数、精确匹配
auto opts = makeNumericScanOptions(ScanDataType::INTEGER32, ScanMatchType::MATCHEQUALTO);
opts.step = 1; // 每字节检查

// 2) 用户值
UserValue uv{};
uv.s32 = 42; // 要查找的整数

// 3) 执行扫描
auto statsOrErr = scanner.performScan(opts, &uv);
if (!statsOrErr) {
	// 失败处理：statsOrErr.error()
}

// 4) 解析结果：从 swath 和 data 计算地址
const auto& matches = scanner.getMatches();
for (const auto& swath : matches.swaths) {
	for (std::size_t i = 0; i < swath.data.size(); ++i) {
		if (swath.data[i].matchInfo != MatchFlags::EMPTY) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
			auto addr = reinterpret_cast<void*>(
					static_cast<std::uintptr_t>(reinterpret_cast<std::uintptr_t>(swath.firstByteInChild)) + i);
			// 使用 addr，例如读取或写入目标进程内存
		}
	}
}
```

实现提示：如果你需要把 `swath` 中的字节按 32-bit 合并并解释为整型，请确保按目标进程的字节序（以及 `opts.reverseEndianness`）正确地组合字节。

---

## 字符串与字节数组扫描

- 字符串：调用 `makeStringScanOptions()` 并在 `UserValue::stringValue` 中设置目标字符串，`UserValue.flags` 可用于指示有效类型。
- 字节数组：在 `UserValue::bytearrayValue` 中设置要匹配的字节序列；`byteMask` 可用于设置通配/掩码位。

---

## 返回统计（ScanStats）字段说明

- `regionsVisited`：扫描时访问的内存映射区域数量。
- `bytesScanned`：扫描过程中实际读取的字节总数。
- `matches`：扫描阶段统计到的匹配总数（便于快速了解匹配规模）。

---

## 错误、权限与局限

- 权限：读取任意进程内存需要 root 或 CAP_SYS_PTRACE 权限；`performScan` 会在权限不足时返回错误字符串。
- I/O/边界：读取 `/proc/<pid>/mem` 可能返回 `EIO/EACCES` 等 I/O 错误，需在上层捕获并记录。
- 跨块匹配：当前引擎以块为单位读取并按步长逐字节匹配，跨块的长模式可能会被遗漏（长匹配需特殊处理）。

---

## 建议的调用流程

1. 使用 `makeNumericScanOptions()` 或 `getDefaultScanOptions()` 构建基础选项。  
2. 对可能的值进行一次宽松扫描以获得候选集合。  
3. 针对候选集合再做更严格/多次过滤以定位单个地址（或更小的集合）。  
4. 在写入或篡改内存前，用 `getMatchCount()` / `getMatches()` 验证地址准确性，并在需要时再次确认（避免误操作）。

---

如果你希望把此文档也生成英文版、或增加“如何将 `swath` 字节解码为 int/float 的示例代码片段”，我可以接着添加。
