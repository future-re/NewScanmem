/**
 * @file write.cppm
 * @brief Write command: modify memory at matched addresses
 */

module;

#include <sys/uio.h>
#include <unistd.h>

#include <charconv>
#include <cstdint>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <vector>

export module cli.commands.write;

import cli.command;
import cli.session;
import ui.show_message;
import core.scanner;
import value.flags;

export namespace cli::commands {

class WriteCommand : public Command {
   public:
    explicit WriteCommand(SessionState& session) : m_session(&session) {}

    [[nodiscard]] auto getName() const -> std::string_view override {
        return "write";
    }

    [[nodiscard]] auto getDescription() const -> std::string_view override {
        return "Write value to memory at matched addresses";
    }

    [[nodiscard]] auto getUsage() const -> std::string_view override {
        return "write <value> [index]\n"
               "  value: 要写入的值 (支持十六进制 0x...)\n"
               "  index (可选): 匹配索引 (默认: 写入所有匹配)\n"
               "  示例: write 100 / write 0xff 0";
    }

    [[nodiscard]] auto validateArgs(const std::vector<std::string>& args) const
        -> std::expected<void, std::string> override {
        if (args.empty()) {
            return std::unexpected("Usage: write <value> [index]");
        }
        return {};
    }

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override {
        if (m_session == nullptr || m_session->pid <= 0) {
            return std::unexpected("Set target pid first: pid <pid>");
        }
        if (!m_session->scanner) {
            return std::unexpected("No matches. Run a scan first.");
        }

        // 解析值
        int64_t value = 0;
        const auto& valueStr = args[0];
        if (valueStr.size() > 2 && valueStr[0] == '0' &&
            (valueStr[1] == 'x' || valueStr[1] == 'X')) {
            auto [ptr, ec] =
                std::from_chars(valueStr.data() + 2,
                                valueStr.data() + valueStr.size(), value, 16);
            if (ec != std::errc{}) {
                return std::unexpected("Invalid hex value: " + valueStr);
            }
        } else {
            auto [ptr, ec] = std::from_chars(
                valueStr.data(), valueStr.data() + valueStr.size(), value, 10);
            if (ec != std::errc{}) {
                return std::unexpected("Invalid value: " + valueStr);
            }
        }

        // 解析索引（可选）
        std::optional<size_t> targetIndex;
        if (args.size() > 1) {
            try {
                targetIndex = std::stoull(args[1]);
            } catch (...) {
                return std::unexpected("Invalid index: " + args[1]);
            }
        }

        auto* scanner = m_session->scanner.get();
        const auto& matches = scanner->getMatches();

        size_t currentIndex = 0;
        size_t writeCount = 0;

        for (const auto& swath : matches.swaths) {
            auto* base = static_cast<std::uint8_t*>(swath.firstByteInChild);
            for (size_t i = 0; i < swath.data.size(); ++i) {
                const auto& cell = swath.data[i];
                if (cell.matchInfo == MatchFlags::EMPTY) {
                    continue;
                }

                if (targetIndex && currentIndex != *targetIndex) {
                    currentIndex++;
                    continue;
                }

                auto addr = std::bit_cast<std::uintptr_t>(base + i);

                // 使用 process_vm_writev 写入内存
                auto byte = static_cast<std::uint8_t>(value & 0xFF);
                iovec local{.iov_base = &byte, .iov_len = 1};
                iovec remote{.iov_base = std::bit_cast<void*>(addr),
                             .iov_len = 1};

                ssize_t written =
                    process_vm_writev(m_session->pid, &local, 1, &remote, 1, 0);

                if (written == 1) {
                    writeCount++;
                    ui::MessagePrinter::info(std::format(
                        "Written 0x{:02x} to 0x{:016x}", byte, addr));
                } else {
                    ui::MessagePrinter::warn(
                        std::format("Failed to write to 0x{:016x}", addr));
                }

                if (targetIndex) {
                    break;  // 只写一个
                }
                currentIndex++;
            }
            if (targetIndex && writeCount > 0) {
                break;
            }
        }

        if (writeCount == 0) {
            return std::unexpected("No values written");
        }

        ui::MessagePrinter::success(
            std::format("Successfully wrote {} value(s)", writeCount));

        return CommandResult{.success = true, .message = ""};
    }

   private:
    SessionState* m_session;
};

}  // namespace cli::commands
