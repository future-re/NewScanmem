#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
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
    static volatile std::uint64_t kMarker = 1122334455667788ULL;

    // 解析参数 --duration-ms N
    int durationMs = 5000;  // 默认 5 秒
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::strcmp(argv[i], "--duration-ms") == 0) {
            durationMs = std::atoi(argv[i + 1]);
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
    std::cout << std::hex << "MARKER: 0x" << kMarker << std::dec << "\n";
    std::cout << "Running for ~" << durationMs << " ms..." << std::endl;

    const auto kDeadline = std::chrono::steady_clock::now() +
                           std::chrono::milliseconds(durationMs);
    while (gRunning.load() && std::chrono::steady_clock::now() < kDeadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "Target finished." << std::endl;
    return 0;
}
