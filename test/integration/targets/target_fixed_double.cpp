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

    // 解析运行时长参数
    int durationMs = 5000;  // 默认5秒
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--duration-ms") == 0 && i + 1 < argc) {
            durationMs = std::atoi(argv[i + 1]);
            break;
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
    auto endTime = startTime + std::chrono::milliseconds(durationMs);

    while (gRunning.load() && std::chrono::steady_clock::now() < endTime) {
        std::cout << "MARKER: " << kMarker << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "Target finished." << std::endl;
    return 0;
}
