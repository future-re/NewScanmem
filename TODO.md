# TODO

## 性能优化

1. **字节搜索优化**：
   - **任务**：
     - 实现 Boyer–Moore–Horspool 算法（支持精确匹配与掩码版本）。
     - 对短模式仍使用 `std::search`。
     - 长模式路径尝试使用 SIMD 或 `memcmp` 分块优化。
   - **目标文件**：
     - scanroutines.cppm：实现搜索算法。
     - test_scanroutines.cpp：添加单元测试。

2. **仅比较 mask=0xFF 的位/字节索引以减少比较开销**：
   - **任务**：
     - 优化掩码匹配逻辑，减少无效比较。
   - **目标文件**：
     - scanroutines.cppm：优化掩码匹配逻辑。
     - test_scanroutines.cpp：验证掩码匹配的正确性。

## API/易用性

1. **轻量解析器**：
   - **任务**：
     - 添加可选的轻量解析器，将 `'?'` 文本占位符解析为掩码（仅在需要时启用）。
   - **目标文件**：
     - scanroutines.cppm：实现解析器。
     - test_scanroutines.cpp：验证解析器功能。

2. **多模式匹配**：
   - **任务**：
     - 支持一次性匹配多条模式。
     - 未来可考虑实现 Aho–Corasick 算法或分桶优化。
   - **目标文件**：
     - scanroutines.cppm：实现多模式匹配逻辑。
     - test_scanroutines.cpp：添加多模式匹配的单元测试。
