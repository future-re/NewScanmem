#include <cstdint>

#include "modification_framework.h"

// 测试目标：数组中的单个元素
// 用于测试扫描和修改数组中的特定元素
auto main(int argc, char** argv) -> int {
    // 定义一个包含多个值的数组
    // NOLINTNEXTLINE
    static volatile std::int32_t kArray[10] = {100, 200, 300, 400, 500,
                                               600, 700, 800, 900, 1000};

    // 监控第5个元素（索引4，值为500）
    constexpr int TARGET_INDEX = 4;
    constexpr std::int32_t EXPECTED_VALUE = 500;
    constexpr std::int32_t MODIFIED_VALUE = 9999;

    // 打印数组内容
    std::cout << "Array contents:" << std::endl;
    for (int i = 0; i < 10; ++i) {
        std::cout << "  kArray[" << i << "] = " << kArray[i]
                  << std::endl;  // NOLINT
    }
    std::cout << "Monitoring kArray[" << TARGET_INDEX << "] for modification..."
              << std::endl;

    // 使用通用框架运行测试（监控数组中的特定元素）
    return modification_framework::runModificationTest(
        argc, argv, kArray[TARGET_INDEX], EXPECTED_VALUE, MODIFIED_VALUE);
}
