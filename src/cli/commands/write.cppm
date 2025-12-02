/**
 * @file write.cppm
 * @brief Write command: modify memory at matched addresses
 */

module;

#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

export module cli.commands.write;

import cli.command;
import cli.session;
import ui.show_message;
import core.memory_writer;
import utils.endianness;
import value.parser;

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
        const auto& valueStr = args[0];
        auto valueOpt = value::parseInt64(valueStr);
        if (!valueOpt) {
            return std::unexpected("Invalid value: " + valueStr);
        }
        auto value = static_cast<std::uint64_t>(*valueOpt);

        // 解析索引（可选）
        std::optional<size_t> targetIndex;
        if (args.size() > 1) {
            try {
                targetIndex = std::stoull(args[1]);
            } catch (...) {
                return std::unexpected("Invalid index: " + args[1]);
            }
        }

        // 创建写入器并执行写入（按会话端序）
        core::MemoryWriter writer(m_session->pid,
                      (m_session->endianness == utils::Endianness::LITTLE
                           ? utils::Endianness::LITTLE
                           : utils::Endianness::BIG));
        auto* scanner = m_session->scanner.get();

        if (targetIndex) {
            // 写入单个匹配
            auto result = writer.writeToMatch(*scanner, value, *targetIndex);
            if (!result) {
                return std::unexpected("Write failed: " + result.error());
            }
            ui::MessagePrinter::success(std::format(
                "Successfully wrote value to match #{}", *targetIndex));
        } else {
            // 批量写入所有匹配
            auto result = writer.writeToMatches(*scanner, value);

            if (result.successCount == 0) {
                if (!result.errors.empty()) {
                    return std::unexpected("All writes failed. First error: " +
                                           result.errors[0]);
                }
                return std::unexpected("No values written");
            }

            // 显示结果摘要
            ui::MessagePrinter::success(std::format(
                "Successfully wrote {} value(s)", result.successCount));

            if (result.failedCount > 0) {
                ui::MessagePrinter::warn(
                    std::format("{} write(s) failed", result.failedCount));

                // 显示前几个错误
                size_t errorLimit = std::min(result.errors.size(), size_t{3});
                for (size_t i = 0; i < errorLimit; ++i) {
                    ui::MessagePrinter::warn("  " + result.errors[i]);
                }
                if (result.errors.size() > errorLimit) {
                    ui::MessagePrinter::warn(
                        std::format("  ... and {} more errors",
                                    result.errors.size() - errorLimit));
                }
            }
        }

        return CommandResult{.success = true, .message = ""};
    }

   private:
    SessionState* m_session;
};

}  // namespace cli::commands
