#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>
#ifdef __unix__
#include <unistd.h>
#endif

namespace {
std::atomic<bool> gRunning{true};

void HandleSignal(int /*unused*/) { gRunning.store(false); }
}  // namespace

auto main(int argc, char** argv) -> int {
    // 可扫描的固定值（保持稳定，便于外部扫描定位）
    static volatile double kMarker = 12345.6789;
    constexpr double kExpectedValue = 12345.6789;
    constexpr double kModifiedValue = 99999.9999;  // 期望被修改成的值
    constexpr double kTolerance = 0.001;           // 浮点数比较容差

    // 解析运行时长参数
    int durationMs = 5000;        // 默认5秒
    int waitForModifyMs = 10000;  // 等待修改的超时时间（默认10秒）
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--duration-ms") == 0 && i + 1 < argc) {
            durationMs = std::atoi(argv[i + 1]);
        } else if (std::strcmp(argv[i], "--wait-modify-ms") == 0 &&
                   i + 1 < argc) {
            waitForModifyMs = std::atoi(argv[i + 1]);
        }
    }

    // 安装信号处理，便于中断退出
    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);

#ifdef __unix__
    std::cout << "PID: " << getpid() << "\n";
#else
    std::cout << "PID: (unknown on non-unix)\n";
#endif
    std::cout << "MARKER: " << kMarker << "\n";

    auto startTime = std::chrono::steady_clock::now();
    auto modifyDeadline =
        startTime + std::chrono::milliseconds(waitForModifyMs);
    auto endTime = startTime + std::chrono::milliseconds(durationMs);

    bool valueModified = false;
    std::cout << "Waiting for value modification..." << std::endl;

    // 阶段1: 等待值被修改
    while (gRunning.load() &&
           std::chrono::steady_clock::now() < modifyDeadline) {
        double currentValue = kMarker;

        // 检查值是否被修改为期望值
        if (std::abs(currentValue - kModifiedValue) < kTolerance) {
            std::cout << "SUCCESS: Value modified from " << kExpectedValue
                      << " to " << currentValue << std::endl;
            valueModified = true;
            break;
        }

        std::cout << "MARKER: " << currentValue << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if (!valueModified) {
        std::cerr << "FAILED: Value was not modified within timeout"
                  << std::endl;
        return 1;  // 失败退出
    }

    // 阶段2: 值已修改，继续运行直到 duration 结束（可选）
    std::cout << "Value successfully modified, continuing..." << std::endl;
    while (gRunning.load() && std::chrono::steady_clock::now() < endTime) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "Target finished successfully." << std::endl;
    return 0;  // 成功退出
}
