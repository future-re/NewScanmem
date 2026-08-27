#include "newscanmem/cli/commands/watch.hpp"

#include <sys/uio.h>

#include <bit>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <format>
#include <thread>

#include "newscanmem/ui/show_message.hpp"

namespace cli::commands {

WatchCommand::WatchCommand(SessionState& session) : m_session(&session) {}
auto WatchCommand::getName() const -> std::string_view { return "watch"; }
auto WatchCommand::getDescription() const -> std::string_view {
    return "Monitor specific address for value changes";
}
auto WatchCommand::getUsage() const -> std::string_view {
    return "watch <address> [interval_ms]";
}
auto WatchCommand::execute(const std::vector<std::string>& args)
    -> std::expected<CommandResult, std::string> {
    if (m_session == nullptr || m_session->pid <= 0)
        return std::unexpected("Set target pid first: pid <pid>");
    if (args.empty())
        return std::unexpected("Usage: watch <address> [interval_ms]");
    const auto& addressString = args[0];
    std::string_view addressView = addressString;
    int base = 10;
    if (addressView.starts_with("0x") || addressView.starts_with("0X")) {
        addressView.remove_prefix(2);
        base = 16;
    }
    std::uintptr_t address = 0;
    const auto parse =
        std::from_chars(addressView.data(),
                        addressView.data() + addressView.size(), address, base);
    if (parse.ec != std::errc{} ||
        parse.ptr != addressView.data() + addressView.size())
        return std::unexpected(
            std::format("Invalid address: {}", addressString));
    int intervalMs = 1000;
    if (args.size() >= 2) {
        const auto interval = std::from_chars(
            args[1].data(), args[1].data() + args[1].size(), intervalMs);
        if (interval.ec != std::errc{} ||
            interval.ptr != args[1].data() + args[1].size() || intervalMs <= 0)
            return std::unexpected(
                std::format("Invalid interval: {}", args[1]));
    }
    ui::MessagePrinter::info(
        std::format("Watching 0x{:016x} (Ctrl+C to stop)...", address));
    std::uint64_t lastValue = 0;
    bool firstRead = true;
    try {
        while (true) {
            std::uint64_t currentValue = 0;
            iovec local{.iov_base = &currentValue,
                        .iov_len = sizeof(currentValue)};
            iovec remote{.iov_base = std::bit_cast<void*>(address),
                         .iov_len = sizeof(currentValue)};
            const ssize_t bytesRead =
                process_vm_readv(m_session->pid, &local, 1, &remote, 1, 0);
            if (bytesRead != sizeof(currentValue)) {
                ui::MessagePrinter::error(
                    std::format("Failed to read memory at 0x{:016x}", address));
                break;
            }
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
            std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));
        }
    } catch (const std::exception& error) {
        return std::unexpected(
            std::format("Watch interrupted: {}", error.what()));
    }
    return CommandResult{.shouldExit = false};
}

}  // namespace cli::commands
