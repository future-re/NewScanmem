# 字节比较与搜索（scan.bytes）

该模块为扫描引擎提供字节级别的比较与搜索工具。以下是面向开发者的 API 参考（函数签名、参数、返回、行为、注意事项和示例）。

---

## API 总览

- `compareBytes` (重载)
- `compareBytesMasked` (重载)
- `findBytePattern` (重载)
- `findBytePatternMasked` (重载)
- `makeBytearrayRoutine`

---

## 类型与约定

- `Mem64`：用于表示和保存缓冲区内容；提供 `bytes()` 返回 `std::span<const uint8_t>`（只读视图）。
- `MatchFlags`：匹配标志，常见位：`B8/B16/B32/B64/STRING/BYTE_ARRAY`，用来标记匹配宽度与类别。
- 语义：`compare*` 函数是“前缀匹配”，`find*` 函数是“搜索”。

---

## compareBytes (pointer+size)

签名:

```cpp
export inline auto compareBytes(const Mem64* memoryPtr,
                                size_t memLength,
                                const uint8_t* patternData,
                                size_t patternSize,
                                MatchFlags* saveFlags) -> unsigned int;
```

说明：判断 `memoryPtr` 指向的缓冲区（限制长度为 `memLength`）从当前位置是否前缀匹配 `patternData[0..patternSize-1]`。

参数：
- `memoryPtr`：指向 `Mem64` 的指针（只读）；不能为空。
- `memLength`：扫描时允许访问的最大字节数（通常为 `region`、`block` 的长度），实际比较长度为 `min(memLength, Mem64::size())`。
- `patternData, patternSize`：要匹配的字节数组（指针 + 长度）。
- `saveFlags`：输出参数；当匹配成功时会写入 `MatchFlags`（至少包含 `B8`）。

返回：匹配长度（即 patternSize）表示匹配成功，若返回 0 表示无匹配或错误（如 patternSize==0 或 patternData==nullptr 或 mem 长度不足）。

复杂度：O(patternSize)

注意：该函数不会搜索缓冲区中间位置；如需搜索请使用 `findBytePattern`。

示例：

```cpp
const std::vector<uint8_t> pattern{0xAA};
MatchFlags flags = MatchFlags::EMPTY;
unsigned matched = compareBytes(&mem, mem.size(), pattern.data(), pattern.size(), &flags);
```

---

## compareBytes (std::vector 重载)

签名：

```cpp
export inline auto compareBytes(const Mem64* memoryPtr,
                                size_t memLength,
                                const std::vector<uint8_t>& pattern,
                                MatchFlags* saveFlags) -> unsigned int;
```

说明：和 `pointer+size` 版本语义相同；只是提供更友好的调用接口。

---

## compareBytesMasked (pointer+size)

签名：

```cpp
export inline auto compareBytesMasked(const Mem64* memoryPtr,
                                      size_t memLength,
                                      const uint8_t* patternData,
                                      size_t patternSize,
                                      const uint8_t* maskData,
                                      size_t maskSize,
                                      MatchFlags* saveFlags) -> unsigned int;
```

说明：带掩码的前缀比较。每个字节以 `((hay ^ pattern) & mask) == 0` 的规则判断。

参数：
- `maskSize` 必须等于 `patternSize`，否则返回 0（不匹配）。
- `maskData` 中比特为 1 的位将进行严格比较（必须相等）；比特为 0 的位将被忽略。

返回：匹配长度（patternSize）或 0。

标志：在匹配成功时 `saveFlags` 会写入 `MatchFlags::B8 | MatchFlags::BYTE_ARRAY`（表明按字节比较且匹配对象为字节数组）。

复杂度：O(patternSize)

示例：

```cpp
const std::vector<uint8_t> pattern{0xAA, 0xBB};
const std::vector<uint8_t> mask{0xFF, 0xF0};
MatchFlags flags = MatchFlags::EMPTY;
auto matched = compareBytesMasked(&mem, mem.size(), pattern.data(), pattern.size(), mask.data(), mask.size(), &flags);
```

实现注意点：
- 逐字节检查 `((hay[j] ^ pattern[j]) & mask[j]) == 0`。

---

## compareBytesMasked (vector 重载)

签名：

```cpp
export inline auto compareBytesMasked(const Mem64* memoryPtr,
                                      size_t memLength,
                                      const std::vector<uint8_t>& pattern,
                                      const std::vector<uint8_t>& mask,
                                      MatchFlags* saveFlags) -> unsigned int;
```

作用同 `pointer+size` 重载。

---

## findBytePattern (pointer+size) & 重载

签名：

```cpp
export inline auto findBytePattern(const Mem64* memoryPtr,
                                   size_t memLength,
                                   const uint8_t* patternData,
                                   size_t patternSize) -> std::optional<ByteMatch>;
```

说明：搜索缓冲区内首次出现 `pattern`，返回 `ByteMatch`（包含 offset 和 length），否则返回 `std::nullopt`。

示例：

```cpp
auto maybeMatch = findBytePattern(&mem, mem.size(), pattern.data(), pattern.size());
if (maybeMatch) {
    // maybeMatch->offset  和 maybeMatch->length
}
```

实现说明：模块使用 `std::ranges::search` 对 `Mem64::bytes()` 视图进行查找。

---

## findBytePatternMasked (pointer+size) & 重载

签名：

```cpp
export inline auto findBytePatternMasked(const Mem64* memoryPtr,
                                         size_t memLength,
                                         const uint8_t* patternData,
                                         size_t patternSize,
                                         const uint8_t* maskData,
                                         size_t maskSize) -> std::optional<ByteMatch>;
```

说明：带掩码的搜索；掩码行为与 `compareBytesMasked` 一致。

注意：此函数在内部对所有可能的起始位置逐字节检查，返回首次匹配的偏移。

复杂度：O(N * M)（缓冲区 N × 模式 M），通常用于短模式或特定场景。

---

## makeBytearrayRoutine

签名：

```cpp
export inline auto makeBytearrayRoutine(ScanMatchType matchType) -> scanRoutine;
```

说明：根据 `ScanMatchType`（例如 `MATCH_EQUAL_TO`, `MATCH_ANY` 等）返回一个 `scanRoutine`。`scanRoutine` 会处理 `UserValue` 中的 `bytearrayValue`、可选 `byteMask`，并在匹配时写入 `MatchFlags`（包括 `B8` 和 `BYTE_ARRAY`）。

示例：

```cpp
UserValue userValue;
userValue.bytearrayValue = std::vector<uint8_t>{0xAA,0xBB};
userValue.byteMask = std::vector<uint8_t>{0xFF, 0xF0};
auto routine = makeBytearrayRoutine(ScanMatchType::MATCH_EQUAL_TO);
MatchFlags flags = MatchFlags::EMPTY;
unsigned matched = routine(&mem, memLength, nullptr, &userValue, &flags);
```

实现说明：`makeBytearrayRoutine` 在逻辑上会调用 `compareBytes` 或 `compareBytesMasked`，并在成功时将 `MatchFlags::BYTE_ARRAY`（用于标记匹配类别）叠加到 `saveFlags`。

---

## 常见问题与边界

- 空模式 (`patternSize==0`)：`compare*` 返回 0；`find*` 返回 `std::nullopt`。
- 掩码与 pattern 长度不同：掩码长度必须与模式相等，否则返回无匹配。
- `saveFlags`：如果 `saveFlags` 为 `nullptr`，函数会尝试写入时可能引发未定义行为；请始终传入有效指针。
