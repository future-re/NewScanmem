# Git 提交说明模板 | Git Commit Message Template

## 标题部分 | Title Section
- 格式：`<type>: <简洁描述> | <Brief description>`
- 示例：
  ```
  feat: 更新代码风格并移除不安全切片方法 | Improve code style and remove unsafe slice method
  ```

## 正文部分 | Body Section
- 详细描述变更内容，使用中英双语。
- 每个变更点使用项目符号（`-`）列出。
- 示例：
  ```
  - 更新了以下模块的代码风格：
    - buffer
    - flags
    - scalar
    Updated code style in the following modules:
    - buffer
    - flags
    - scalar

  - 移除了 view 中的不安全 slice 方法，仅保留安全的 subview 方法。
    Removed the unsafe slice method in view and retained only the subview method for safer slicing.

  - 提高了代码一致性和可维护性。
    Improved code consistency and maintainability.
  ```

## 提交类型 | Commit Types
- `feat`：新增功能或改进
- `fix`：修复问题
- `refactor`：代码重构
- `docs`：文档更新
- `style`：代码格式（不影响功能）
- `test`：添加或修改测试
- `chore`：构建过程或辅助工具的变更

## 使用说明 | Usage Instructions
1. 根据变更内容选择合适的提交类型（如 `feat`、`fix`）。
2. 在标题部分简洁描述变更内容，保持 50 字以内。
3. 在正文部分详细列出变更点，确保中英双语清晰表达。
4. 提交时将模板内容复制到提交消息中，并替换占位内容。