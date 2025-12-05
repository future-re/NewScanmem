#include <cstdint>

#include "modification_framework.h"

// 测试目标：int32 类型值
// 用于测试 int32 类型的扫描和修改
auto main(int argc, char** argv) -> int {
    static volatile std::int32_t kMarker = 2000000000;
    constexpr std::int32_t EXPECTED_VALUE = 2000000000;
    constexpr std::int32_t MODIFIED_VALUE = 1000000000;

    // 使用通用框架运行测试
    return modification_framework::runModificationTest(
        argc, argv, kMarker, EXPECTED_VALUE, MODIFIED_VALUE);
}