#include <cstdint>

#include "modification_framework.h"

auto main(int argc, char** argv) -> int {
    // 可扫描的固定值（保持稳定，便于外部扫描定位）
    static volatile std::uint64_t marker = 1122334455667788ULL;
    constexpr std::uint64_t EXPECTEDVALUE = 1122334455667788ULL;
    constexpr std::uint64_t MODIFIEDVALUE = 9999999999999999ULL;

    // 使用通用框架运行测试
    return modification_framework::runModificationTest(
        argc, argv, marker, EXPECTEDVALUE, MODIFIEDVALUE);
}
