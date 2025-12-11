#ifndef MODIFICATION_FRAMEWORK_H
#define MODIFICATION_FRAMEWORK_H

#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#ifdef __unix__
#include <unistd.h>
#endif

namespace modification_framework {

// 全局运行标志
std::atomic<bool> gRunning{true};  // NOLINT

// 信号处理函数
void handleSignal(int /*unused*/) { gRunning.store(false); }

// 安装信号处理器
inline void installSignalHandlers() {
    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
}

// 打印进程 PID
inline void printPid() {
#ifdef __unix__
    std::cout << "PID: " << getpid() << "\n";
#else
    std::cout << "PID: (unknown on non-unix)\n";
#endif
}

// 解析命令行参数
inline auto parseWaitTimeout(int argc, char** argv, int defaultMs = 10000)
    -> int {
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--wait-modify-ms") == 0 && i + 1 < argc) {
            return std::atoi(argv[++i]);
        }
    }
    return defaultMs;
}

// 打印标记值（通用版本）
template <typename T>
inline void printMarker(const T& marker) {
    std::cout << "Addr:" << &marker << " MARKER: " << marker << "\n";
}

// 打印标记值（十六进制版本，用于整数）
template <typename T>
inline void printMarkerHex(const T& marker) {
    std::cout << std::hex << "Addr:" << &marker << " MARKER: 0x" << marker
              << std::dec << "\n";
}

// 打印标记值（std::string 特化版本）
inline void printMarker(const std::string& marker) {
    std::cout << "Addr:" << static_cast<const void*>(marker.data())
              << " MARKER: \"" << marker << "\"\n";
}

// 打印标记值（char* 特化版本）
inline void printMarker(char* const& marker) {
    std::cout << "Addr:" << static_cast<const void*>(marker) << " MARKER: \""
              << (marker != nullptr ? marker : "(null)") << "\"\n";
}

// 等待值被修改的核心函数
template <typename T>
auto waitForModification(volatile T& marker, T expectedValue, T modifiedValue,
                         int waitTimeoutMs, double tolerance = 0.0) -> bool {
    auto startTime = std::chrono::steady_clock::now();
    auto deadline = startTime + std::chrono::milliseconds(waitTimeoutMs);
    std::cout << "Waiting for value modification..." << std::endl;

    while (gRunning.load() && std::chrono::steady_clock::now() < deadline) {
        T currentValue = marker;

        if constexpr (std::is_floating_point_v<T>) {
            if (std::abs(currentValue - modifiedValue) < tolerance) {
                std::cout << "SUCCESS: Value modified from " << expectedValue
                          << " to " << currentValue << std::endl;
                std::cout << "RESULT: PASS" << std::endl;
                return true;
            }
        } else {
            if (currentValue == modifiedValue) {
                std::cout << "SUCCESS: Value modified from " << expectedValue
                          << " to " << currentValue << std::endl;
                std::cout << "RESULT: PASS" << std::endl;
                return true;
            }
        }

        std::cout << "MARKER: " << currentValue << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cerr << "FAILED: Value was not modified within timeout" << std::endl;
    std::cout << "RESULT: FAIL" << std::endl;
    return false;
}

// 等待值被修改的核心函数（std::string 特化版本）
inline auto waitForModification(std::string& marker,
                                const std::string& expectedValue,
                                const std::string& modifiedValue,
                                int waitTimeoutMs, double /*tolerance*/ = 0.0)
    -> bool {
    auto startTime = std::chrono::steady_clock::now();
    auto deadline = startTime + std::chrono::milliseconds(waitTimeoutMs);
    std::cout << "Waiting for value modification..." << std::endl;

    while (gRunning.load() && std::chrono::steady_clock::now() < deadline) {
        std::string currentValue = marker;

        if (currentValue == modifiedValue) {
            std::cout << "SUCCESS: Value modified from \"" << expectedValue
                      << "\" to \"" << currentValue << "\"" << std::endl;
            std::cout << "RESULT: PASS" << std::endl;
            return true;
        }

        std::cout << "MARKER: \"" << currentValue << "\"" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cerr << "FAILED: Value was not modified within timeout" << std::endl;
    std::cout << "RESULT: FAIL" << std::endl;
    return false;
}

// 等待值被修改的核心函数（char* 特化版本）
inline auto waitForModification(char*& marker, const char* expectedValue,
                                const char* modifiedValue, int waitTimeoutMs,
                                double /*tolerance*/ = 0.0) -> bool {
    auto startTime = std::chrono::steady_clock::now();
    auto deadline = startTime + std::chrono::milliseconds(waitTimeoutMs);
    std::cout << "Waiting for value modification..." << std::endl;

    while (gRunning.load() && std::chrono::steady_clock::now() < deadline) {
        char* currentValue = marker;
        const char* currentStr =
            (currentValue != nullptr) ? currentValue : "(null)";
        const char* modifiedStr = (modifiedValue != nullptr) ? modifiedValue : "(null)";

        if ((currentValue == nullptr && modifiedValue == nullptr) ||
            (currentValue != nullptr && modifiedValue != nullptr &&
             std::strcmp(currentValue, modifiedValue) == 0)) {
            std::cout << "SUCCESS: Value modified from \""
                      << ((expectedValue != nullptr) ? expectedValue : "(null)")
                      << "\" to \"" << currentStr << "\"" << std::endl;
            std::cout << "RESULT: PASS" << std::endl;
            return true;
        }

        std::cout << "MARKER: \"" << currentStr << "\"" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cerr << "FAILED: Value was not modified within timeout" << std::endl;
    std::cout << "RESULT: FAIL" << std::endl;
    return false;
}

// 通用的测试主函数模板
template <typename T>
auto runModificationTest(int argc, char** argv, volatile T& marker,
                         T expectedValue, T modifiedValue,
                         double tolerance = 0.0) -> int {
    // 解析命令行参数
    int waitForModifyMs = parseWaitTimeout(argc, argv);

    // 安装信号处理
    installSignalHandlers();

    // 打印 PID
    printPid();

    // 打印标记值
    if constexpr (std::is_integral_v<T>) {
        printMarkerHex(marker);
    } else {
        printMarker(marker);
    }

    // 等待值修改
    if (!waitForModification(marker, expectedValue, modifiedValue,
                             waitForModifyMs, tolerance)) {
        return 1;  // 失败退出
    }

    std::cout << "Target finished successfully." << std::endl;
    return 0;  // 成功退出
}

// 测试主函数（std::string 特化版本）
inline auto runModificationTest(int argc, char** argv, std::string& marker,
                                const std::string& expectedValue,
                                const std::string& modifiedValue,
                                double tolerance = 0.0) -> int {
    // 解析命令行参数
    int waitForModifyMs = parseWaitTimeout(argc, argv);

    // 安装信号处理
    installSignalHandlers();

    // 打印 PID
    printPid();

    // 打印标记值
    printMarker(marker);

    // 等待值修改
    if (!waitForModification(marker, expectedValue, modifiedValue,
                             waitForModifyMs, tolerance)) {
        return 1;  // 失败退出
    }

    std::cout << "Target finished successfully." << std::endl;
    return 0;  // 成功退出
}

// 测试主函数（char* 特化版本）
inline auto runModificationTest(int argc, char** argv, char*& marker,
                                const char* expectedValue,
                                const char* modifiedValue,
                                double tolerance = 0.0) -> int {
    // 解析命令行参数
    int waitForModifyMs = parseWaitTimeout(argc, argv);

    // 安装信号处理
    installSignalHandlers();

    // 打印 PID
    printPid();

    // 打印标记值
    printMarker(marker);

    // 等待值修改
    if (!waitForModification(marker, expectedValue, modifiedValue,
                             waitForModifyMs, tolerance)) {
        return 1;  // 失败退出
    }

    std::cout << "Target finished successfully." << std::endl;
    return 0;  // 成功退出
}

}  // namespace modification_framework

#endif  // MODIFICATION_FRAMEWORK_H