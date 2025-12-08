# 贡献指南

感谢你的贡献！为了保持项目质量与效率，请遵循以下流程：

- 提交信息格式：建议使用 Conventional Commits，例如 `feat: add numeric scan filter`。
- 分支策略：从 `main` 创建特性分支，提交 PR 至 `main`。
- 代码风格：使用 `clang-format`（LLVM 风格），CI 会进行检查。
- 静态分析：`clang-tidy` 在 CI 中执行，建议本地先修复告警。
- 测试：为新增功能补充单元测试/集成测试，确保 `ctest` 通过。
- 文档：更新 `mkdocs` 对应页面与 `README.md` 快速开始示例。

## 本地构建与测试

```bash
mkdir -p build && cd build
cmake -GNinja ..
ninja
ctest --output-on-failure
```

## 行为准则

请遵循 `CODE_OF_CONDUCT.md` 中的社区行为准则。
