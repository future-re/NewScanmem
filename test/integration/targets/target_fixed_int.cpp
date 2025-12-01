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

    // 安装信号处理，便于中断退出
    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);

#ifdef __unix__
    std::cout << "PID: " << getpid() << "\n";
#else
    std::cout << "PID: (unknown on non-unix)\n";
#endif
    std::cout << std::hex << "MARKER: 0x" << kMarker << std::dec << "\n";

    while (gRunning.load()) {
        std::cout << std::hex << "MARKER: 0x" << kMarker << std::dec
                  << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Target finished." << std::endl;
    return 0;
}
