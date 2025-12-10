#!/bin/bash
# generate_coverage.sh - Generate comprehensive coverage reports
#
# Usage:
#   ./generate_coverage.sh [--html] [--lcov] [--summary] [--all] [--check]
#
# Options:
#   --html      Generate HTML report (default)
#   --lcov      Generate LCOV format for IDE integration
#   --summary   Print text summary to console
#   --all       Generate all formats
#   --check     Check coverage against thresholds (fails if below minimum)
#   --help      Show this help message

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
COVERAGE_DIR="${BUILD_DIR}/coverage"
TEST_DIR="${BUILD_DIR}/test"

# Default options
GENERATE_HTML=true
GENERATE_LCOV=false
GENERATE_SUMMARY=true
CHECK_THRESHOLDS=false

# Coverage thresholds
MIN_LINE_COVERAGE=80
MIN_FUNCTION_COVERAGE=75
MIN_REGION_COVERAGE=70

# Parse arguments
if [ $# -eq 0 ]; then
    GENERATE_HTML=true
    GENERATE_SUMMARY=true
else
    GENERATE_HTML=false
    GENERATE_SUMMARY=false
    
    for arg in "$@"; do
        case $arg in
            --html)
                GENERATE_HTML=true
                ;;
            --lcov)
                GENERATE_LCOV=true
                ;;
            --summary)
                GENERATE_SUMMARY=true
                ;;
            --all)
                GENERATE_HTML=true
                GENERATE_LCOV=true
                GENERATE_SUMMARY=true
                ;;
            --check)
                CHECK_THRESHOLDS=true
                ;;
            --help)
                grep '^#' "$0" | tail -n +2 | head -n -1 | cut -c3-
                exit 0
                ;;
            *)
                echo "Unknown option: $arg"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done
fi

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Error: Build directory not found at $BUILD_DIR"
    echo "Please run cmake configure first"
    exit 1
fi

# Find llvm tools
LLVM_COV=$(which llvm-cov 2>/dev/null || echo "")
LLVM_PROFDATA=$(which llvm-profdata 2>/dev/null || echo "")

if [ -z "$LLVM_COV" ]; then
    echo "Error: llvm-cov not found in PATH"
    exit 1
fi

if [ -z "$LLVM_PROFDATA" ]; then
    echo "Error: llvm-profdata not found in PATH"
    exit 1
fi

echo "=== Coverage Analysis for NewScanmem ==="
echo "Using llvm-cov: $LLVM_COV"
echo "Using llvm-profdata: $LLVM_PROFDATA"
echo ""

# Create coverage directory
mkdir -p "$COVERAGE_DIR"

# Step 1: Run tests
echo "Step 1/4: Running tests..."
cd "$BUILD_DIR"
LLVM_PROFILE_FILE="$TEST_DIR/%p.profraw" ctest --output-on-failure
echo "✓ Tests completed"
echo ""

# Step 2: Merge profile data
echo "Step 2/4: Merging coverage data..."
PROFRAW_FILES=("$TEST_DIR"/*.profraw)
if [ ${#PROFRAW_FILES[@]} -eq 0 ] || [ ! -f "${PROFRAW_FILES[0]}" ]; then
    echo "Error: No .profraw files found in $TEST_DIR"
    echo "Make sure tests were built with coverage flags enabled:"
    echo "  cmake -B build -DENABLE_COVERAGE=ON"
    exit 1
fi

$LLVM_PROFDATA merge -sparse "$TEST_DIR"/*.profraw -o "$COVERAGE_DIR/coverage.profdata"
echo "✓ Merged $(ls -1 "$TEST_DIR"/*.profraw | wc -l) profile files"
echo ""

# Step 3: Collect all test binaries
echo "Step 3/4: Collecting test binaries..."
TEST_BINARIES=()
for binary in "$TEST_DIR"/test_*; do
    if [ -x "$binary" ] && [ ! -d "$binary" ]; then
        TEST_BINARIES+=("$binary")
    fi
done

if [ ${#TEST_BINARIES[@]} -eq 0 ]; then
    echo "Error: No test binaries found in $TEST_DIR"
    exit 1
fi

echo "✓ Found ${#TEST_BINARIES[@]} test binaries"
echo ""

# Build llvm-cov arguments
OBJECT_ARGS=()
for binary in "${TEST_BINARIES[@]}"; do
    OBJECT_ARGS+=("-object" "$binary")
done

# Source filters (only show coverage for src/ directory)
SOURCE_FILTER="${SCRIPT_DIR}/src"

# Step 4: Generate reports
echo "Step 4/4: Generating coverage reports..."

if [ "$GENERATE_HTML" = true ]; then
    echo "  → Generating HTML report..."
    $LLVM_COV show \
        -instr-profile="$COVERAGE_DIR/coverage.profdata" \
        "${TEST_BINARIES[0]}" \
        "${OBJECT_ARGS[@]:2}" \
        -format=html \
        -output-dir="$COVERAGE_DIR/html" \
        -show-line-counts-or-regions \
        -show-instantiation-summary \
        -ignore-filename-regex='test/|build/|cmake-' \
        "$SOURCE_FILTER"
    echo "  ✓ HTML report: file://$COVERAGE_DIR/html/index.html"
fi

if [ "$GENERATE_SUMMARY" = true ]; then
    echo "  → Generating summary report..."
    $LLVM_COV report \
        -instr-profile="$COVERAGE_DIR/coverage.profdata" \
        "${TEST_BINARIES[0]}" \
        "${OBJECT_ARGS[@]:2}" \
        -ignore-filename-regex='test/|build/|cmake-' \
        "$SOURCE_FILTER" | tee "$COVERAGE_DIR/summary.txt"
fi

if [ "$GENERATE_LCOV" = true ]; then
    echo "  → Generating LCOV format..."
    $LLVM_COV export \
        -instr-profile="$COVERAGE_DIR/coverage.profdata" \
        "${TEST_BINARIES[0]}" \
        "${OBJECT_ARGS[@]:2}" \
        -format=lcov \
        -ignore-filename-regex='test/|build/|cmake-' \
        > "$COVERAGE_DIR/coverage.lcov"
    echo "  ✓ LCOV file: $COVERAGE_DIR/coverage.lcov"
fi

echo ""
echo "=== Coverage Analysis Complete ==="

# Check thresholds
if [ "$CHECK_THRESHOLDS" = true ]; then
    echo ""
    echo "=== Checking Coverage Thresholds ==="
    
    # Extract coverage percentages from summary
    SUMMARY=$($LLVM_COV report \
        -instr-profile="$COVERAGE_DIR/coverage.profdata" \
        "${TEST_BINARIES[0]}" \
        "${OBJECT_ARGS[@]:2}" \
        -ignore-filename-regex='test/|build/|cmake-' \
        "$SOURCE_FILTER")
    
    # Parse total coverage from last line
    TOTAL_LINE=$(echo "$SUMMARY" | tail -n 1)
    
    LINE_COV=$(echo "$TOTAL_LINE" | awk '{print $10}' | tr -d '%')
    FUNC_COV=$(echo "$TOTAL_LINE" | awk '{print $3}' | tr -d '%')
    REGION_COV=$(echo "$TOTAL_LINE" | awk '{print $7}' | tr -d '%')
    
    echo "Line Coverage:     ${LINE_COV}% (minimum: ${MIN_LINE_COVERAGE}%)"
    echo "Function Coverage: ${FUNC_COV}% (minimum: ${MIN_FUNCTION_COVERAGE}%)"
    echo "Region Coverage:   ${REGION_COV}% (minimum: ${MIN_REGION_COVERAGE}%)"
    
    FAILED=false
    
    if (( $(echo "$LINE_COV < $MIN_LINE_COVERAGE" | bc -l) )); then
        echo "✗ Line coverage below threshold"
        FAILED=true
    fi
    
    if (( $(echo "$FUNC_COV < $MIN_FUNCTION_COVERAGE" | bc -l) )); then
        echo "✗ Function coverage below threshold"
        FAILED=true
    fi
    
    if (( $(echo "$REGION_COV < $MIN_REGION_COVERAGE" | bc -l) )); then
        echo "✗ Region coverage below threshold"
        FAILED=true
    fi
    
    if [ "$FAILED" = true ]; then
        echo ""
        echo "Coverage check FAILED - coverage is below minimum thresholds"
        exit 1
    else
        echo ""
        echo "✓ Coverage check PASSED - all thresholds met"
    fi
fi

echo ""
echo "Coverage reports available at:"
if [ "$GENERATE_HTML" = true ]; then
    echo "  HTML:    file://$COVERAGE_DIR/html/index.html"
fi
if [ "$GENERATE_LCOV" = true ]; then
    echo "  LCOV:    $COVERAGE_DIR/coverage.lcov"
fi
if [ "$GENERATE_SUMMARY" = true ]; then
    echo "  Summary: $COVERAGE_DIR/summary.txt"
fi

# 如果存在 mkdocs 的复制脚本，则将生成的 HTML 报告复制到 mkdocs docs 目录
COPY_SCRIPT="$SCRIPT_DIR/mkdocs/scripts/copy_coverage_html.sh"
if [ -x "$COPY_SCRIPT" ] && [ -d "$COVERAGE_DIR/html" ]; then
    echo ""
    echo "Copying coverage HTML into mkdocs docs using $COPY_SCRIPT..."
    # 以独立进程运行脚本（脚本本身会计算仓库根目录）
    "$COPY_SCRIPT"
    echo "✓ Copied coverage HTML into mkdocs/docs/coverage_html"
fi
