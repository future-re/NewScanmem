#include <cstdint>
#include <iostream>
#include <numbers>

#include "modification_framework.h"

// 测试目标：多个独立的值
// 程序保持运行，便于用户测试扫描多个不同的值
auto main(int argc, char** argv) -> int {
    // 定义多个不同类型和值的变量
    static volatile std::int32_t value1 = 12345;
    static volatile std::int32_t value2 = 67890;
    static volatile std::int64_t value3 = 9876543210LL;
    static volatile float value4 = std::numbers::pi_v<float>;
    static volatile double value5 = std::numbers::e;

    // 解析命令行参数
    int waitForModifyMs = modification_framework::parseWaitTimeout(argc, argv);

    // 安装信号处理
    modification_framework::installSignalHandlers();

    // 打印 PID
    modification_framework::printPid();

    // 打印所有值
    std::cout << "Multiple values for scanning:" << std::endl;
    std::cout << "  VALUE1 (int32):  " << value1 << std::endl;
    std::cout << "  VALUE2 (int32):  " << value2 << std::endl;
    std::cout << "  VALUE3 (int64):  " << value3 << std::endl;
    std::cout << "  VALUE4 (float):  " << value4 << std::endl;
    std::cout << "  VALUE5 (double): " << value5 << std::endl;
    std::cout << "\nProgram will run for " << waitForModifyMs / 1000
              << " seconds." << std::endl;
    std::cout << "You can scan and modify any of these values manually."
              << std::endl;

    // 持续运行，等待用户手动测试
    auto startTime = std::chrono::steady_clock::now();
    auto deadline = startTime + std::chrono::milliseconds(waitForModifyMs);

    while (modification_framework::gRunning.load() &&
           std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // 定期打印当前值
        std::cout << "Current values: " << value1 << ", " << value2 << ", "
                  << value3 << ", " << value4 << ", " << value5 << std::endl;
    }

    std::cout << "Target finished." << std::endl;
    return 0;
}
