#!/bin/bash
# 生成 API 文档脚本

set -e

echo "🔧 清理旧文档..."
rm -rf mkdocs/docs/doxygen-output

echo "📚 运行 Doxygen 生成 API 文档..."
doxygen Doxyfile

echo "✅ API 文档生成完成！"
echo "📂 输出位置: mkdocs/docs/doxygen-output/html/"
echo ""
echo "💡 提示："
echo "   - 本地预览: cd mkdocs && mkdocs serve"
echo "   - 查看 API: http://localhost:8000/api/"
