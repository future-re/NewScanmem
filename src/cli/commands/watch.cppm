/**
 * @file watch.cppm
 * @brief Watch command: monitor specific address for value changes
 */

module;

#include <sys/uio.h>
#include <unistd.h>

#include <bit>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

export module cli.commands.watch;

import cli.command;
import cli.session;
import ui.show_message;

export namespace cli::commands {

class WatchCommand : public Command {
   public:
    explicit WatchCommand(SessionState& session) : m_session(&session) {}

    [[nodiscard]] auto getName() const -> std::string_view override {
        return "watch";
    }

    [[nodiscard]] auto getDescription() const -> std::string_view override {
        return "Monitor specific address for value changes";
    }

    [[nodiscard]] auto getUsage() const -> std::string_view override {
        return "watch <address> [interval_ms]";
    }

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override {
        if (m_session == nullptr || m_session->pid <= 0) {
            return std::unexpected("Set target pid first: pid <pid>");
        }

        if (args.empty()) {
            return std::unexpected("Usage: watch <address> [interval_ms]");
        }

        // 解析地址
        const auto& addrStr = args[0];
        std::uintptr_t addr = 0;

        // 支持 0x 前缀
        std::string_view sv = addrStr;
        int base = 10;
        if (sv.starts_with("0x") || sv.starts_with("0X")) {
            sv.remove_prefix(2);
            base = 16;
        }

        auto result =
            std::from_chars(sv.data(), sv.data() + sv.size(), addr, base);
        if (result.ec != std::errc{}) {
            return std::unexpected(std::format("Invalid address: {}", addrStr));
        }

        // 解析间隔（可选）
        int intervalMs = 1000;
        if (args.size() >= 2) {
            auto result2 = std::from_chars(
                args[1].data(), args[1].data() + args[1].size(), intervalMs);
            if (result2.ec != std::errc{} || intervalMs <= 0) {
                return std::unexpected(
                    std::format("Invalid interval: {}", args[1]));
            }
        }

        ui::MessagePrinter::info(
            std::format("Watching 0x{:016x} (Ctrl+C to stop)...", addr));

        // 监控循环
        std::uint64_t lastValue = 0;
        bool firstRead = true;

        try {
            while (true) {
                // 读取当前值
                std::uint64_t currentValue = 0;
                iovec local{.iov_base = &currentValue,
                            .iov_len = sizeof(currentValue)};
                iovec remote{.iov_base = std::bit_cast<void*>(addr),
                             .iov_len = sizeof(currentValue)};

                ssize_t bytesRead =
                    process_vm_readv(m_session->pid, &local, 1, &remote, 1, 0);

                if (bytesRead != sizeof(currentValue)) {
                    ui::MessagePrinter::error(std::format(
                        "Failed to read memory at 0x{:016x}", addr));
                    break;
                }

                // 检测变化
                if (firstRead) {
                    ui::MessagePrinter::info(
                        std::format("Initial value: 0x{:016x}", currentValue));
                    lastValue = currentValue;
                    firstRead = false;
                } else if (currentValue != lastValue) {
                    ui::MessagePrinter::info(
                        std::format("Value changed: 0x{:016x} -> 0x{:016x}",
                                    lastValue, currentValue));
                    lastValue = currentValue;
                }

                // 等待
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(intervalMs));
            }
        } catch (const std::exception& e) {
            return std::unexpected(
                std::format("Watch interrupted: {}", e.what()));
        }

        return CommandResult{.shouldExit = false};
    }

   private:
    SessionState* m_session;
};

}  // namespace cli::commands
