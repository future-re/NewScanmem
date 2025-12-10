# MkDocs 本地文档与覆盖率集成

本目录包含用于构建 MkDocs 文档站点的辅助脚本。

主要说明：

- `scripts/copy_coverage_html.sh`：把 `build/coverage/html` 复制到 `mkdocs/docs/coverage_html`，以便把完整的 HTML 覆盖率报告嵌入到文档页面中（`coverage.md` 使用 iframe 嵌入）。
- `generate_coverage.sh`：现在会在生成覆盖率后自动尝试运行上述复制脚本（如果存在且可执行）。

使用步骤：

1. 生成覆盖率并复制到文档目录：

```bash
cd /path/to/repo
./generate_coverage.sh --all
```

2. 本地预览文档：

```bash
cd mkdocs
mkdocs serve
# 打开 http://127.0.0.1:8000/coverage/ 查看嵌入页面
```

在 CI 中，你可以在构建文档之前运行 `./generate_coverage.sh --all`，以确保 `coverage_html` 已经包含在文档构建中。
