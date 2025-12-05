#!/usr/bin/env bash
set -uo pipefail

# 说明：以 sudo 方式运行此脚本，逐个执行集成测试
# 用法：
#   sudo bash test/integration/run_all_tests.sh

ROOT_DIR="$(cd "$(dirname "$0")"/../.. && pwd)"
BUILD_DIR="$ROOT_DIR/build"
SCRIPT_DIR="$ROOT_DIR/test/integration"
PY_SCRIPT="$SCRIPT_DIR/test_integration.py"
REPORT_DIR="$SCRIPT_DIR"
REPORT_FILE="$REPORT_DIR/integration-report-$(date +%Y%m%d-%H%M%S).txt"

# 颜色定义
COLOR_RESET='\033[0m'
COLOR_BOLD='\033[1m'
COLOR_GREEN='\033[0;32m'
COLOR_RED='\033[0;31m'
COLOR_YELLOW='\033[0;33m'
COLOR_BLUE='\033[0;34m'
COLOR_CYAN='\033[0;36m'

# 美化打印函数
print_header() {
  echo -e "\n${COLOR_BOLD}${COLOR_CYAN}╔═══════════════════════════════════════════════════════════════╗${COLOR_RESET}"
  echo -e "${COLOR_BOLD}${COLOR_CYAN}║  $1${COLOR_RESET}"
  echo -e "${COLOR_BOLD}${COLOR_CYAN}╚═══════════════════════════════════════════════════════════════╝${COLOR_RESET}"
}

print_test_start() {
  echo -e "\n${COLOR_BOLD}${COLOR_BLUE}▶ Running:${COLOR_RESET} ${COLOR_BOLD}$1${COLOR_RESET}"
  echo -e "${COLOR_BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${COLOR_RESET}"
}

print_pass() {
  echo -e "${COLOR_BOLD}${COLOR_GREEN}✓ PASS${COLOR_RESET} $1"
}

print_fail() {
  echo -e "${COLOR_BOLD}${COLOR_RED}✗ FAIL${COLOR_RESET} $1"
}

print_summary() {
  echo -e "\n${COLOR_BOLD}${COLOR_CYAN}╔═══════════════════════════════════════════════════════════════╗${COLOR_RESET}"
  echo -e "${COLOR_BOLD}${COLOR_CYAN}║  Test Summary${COLOR_RESET}"
  echo -e "${COLOR_BOLD}${COLOR_CYAN}╚═══════════════════════════════════════════════════════════════╝${COLOR_RESET}"
}

# 检查构建目录
if [[ ! -d "$BUILD_DIR" ]]; then
  echo "[ERROR] Build directory not found: $BUILD_DIR" >&2
  exit 1
fi

# 检查 Python 测试脚本
if [[ ! -f "$PY_SCRIPT" ]]; then
  echo "[ERROR] Integration python script not found: $PY_SCRIPT" >&2
  exit 1
fi

results=()
passes=0
failures=0

# 初始化报告文件
{
  echo "╔═══════════════════════════════════════════════════════════════╗"
  echo "║  Integration Test Report                                      ║"
  echo "╚═══════════════════════════════════════════════════════════════╝"
  echo ""
  echo "Generated at: $(date -Iseconds)"
  echo "Workspace: $ROOT_DIR"
  echo "Build Dir: $BUILD_DIR"
  echo "Script: $PY_SCRIPT"
  echo ""
} > "$REPORT_FILE"

print_header "NewScanmem Integration Tests"
echo -e "${COLOR_YELLOW}Report will be saved to:${COLOR_RESET} $REPORT_FILE"

run_test() {
  local name="$1"; shift
  
  print_test_start "$name"
  echo "" >> "$REPORT_FILE"
  echo "===== Running: $name =====" >> "$REPORT_FILE"
  
  # 以 sudo 调用 python 脚本，确保能附加进程
  if sudo python3 "$PY_SCRIPT" "$@" 2>&1 | tee -a "$REPORT_FILE"; then
    results+=("$name")
    passes=$((passes+1))
    echo "RESULT: PASS" >> "$REPORT_FILE"
    print_pass "$name"
  else
    results+=("$name")
    failures=$((failures+1))
    echo "RESULT: FAIL" >> "$REPORT_FILE"
    print_fail "$name"
  fi
  
  echo -e "${COLOR_BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${COLOR_RESET}"
}

# 测试列表
run_test "Integration_Scan_Int64" \
  target_fixed_int int64 1122334455667788 9999999999999999 --wait-modify-ms 10000

run_test "Integration_Scan_Double" \
  target_fixed_double float64 12345.6789 99999.9999 --wait-modify-ms 10000

run_test "Integration_Scan_Struct_Field" \
  target_struct_field int32 9999 1000000 --wait-modify-ms 10000

# 可选：运行一个手动值观察目标，不做修改验证，仅便于手工实验
# sudo "$BUILD_DIR/test/integration/target_multiple_values" --wait-modify-ms 15000 || true

print_summary

total=$((passes+failures))
echo ""
for i in "${!results[@]}"; do
  local name="${results[$i]}"
  if [ $i -lt $passes ]; then
    print_pass "$name"
  else
    print_fail "$name"
  fi
done

echo ""
echo -e "${COLOR_BOLD}Total:${COLOR_RESET} $total tests"
echo -e "${COLOR_GREEN}${COLOR_BOLD}Passed:${COLOR_RESET} $passes"
echo -e "${COLOR_RED}${COLOR_BOLD}Failed:${COLOR_RESET} $failures"

# 写入报告文件摘要
{
  echo ""
  echo "╔═══════════════════════════════════════════════════════════════╗"
  echo "║  Summary                                                      ║"
  echo "╚═══════════════════════════════════════════════════════════════╝"
  echo ""
  echo "Total: $total | PASS: $passes | FAIL: $failures"
  echo ""
  for i in "${!results[@]}"; do
    local name="${results[$i]}"
    if [ $i -lt $passes ]; then
      echo "✓ PASS: $name"
    else
      echo "✗ FAIL: $name"
    fi
  done
} >> "$REPORT_FILE"

echo ""
echo -e "${COLOR_YELLOW}${COLOR_BOLD}Report saved to:${COLOR_RESET} ${COLOR_CYAN}$REPORT_FILE${COLOR_RESET}"

if [[ "$failures" -gt 0 ]]; then
  exit 1
fi