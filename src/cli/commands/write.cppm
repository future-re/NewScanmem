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
import core.memory_writer;
import ui.show_message;
import value.core;
import value.parser;
import scan.types;

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

    // NOLINTNEXTLINE(readability-function-cognitive-complexity)
    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override {
        if (m_session == nullptr || m_session->pid <= 0) {
            return std::unexpected("Set target pid first: pid <pid>");
        }
        if (!m_session->scanner) {
            return std::unexpected("No matches. Run a scan first.");
        }

        // 获取最后一次扫描的数据类型
        auto lastDataType = m_session->scanner->getLastDataType();
        if (!lastDataType) {
            return std::unexpected(
                "No scan data type available. Run a scan first.");
        }

        const auto& valueStr = args[0];
        auto value = parseWriteUserValue(*lastDataType, valueStr);
        if (!value) {
            return std::unexpected("Invalid value for current scan type: " +
                                   valueStr);
        }

        // 解析索引
        std::optional<size_t> targetIndex;
        if (args.size() > 1) {
            try {
                targetIndex = std::stoull(args[1]);
            } catch (...) {
                return std::unexpected("Invalid index: " + args[1]);
            }
        }

        core::MemoryWriter writer(m_session->pid, m_session->endianness);
        auto* scanner = m_session->scanner.get();

        std::vector<size_t> writeTargets;
        if (targetIndex) {
            writeTargets.push_back(*targetIndex);
        } else {
            const auto totalMatches = scanner->getMatchCount();
            writeTargets.reserve(totalMatches);
            for (size_t index = 0; index < totalMatches; ++index) {
                writeTargets.push_back(index);
            }
        }

        auto result = writer.writeToMatch(*scanner, *value, writeTargets);
        if (!result) {
            return std::unexpected("Write failed: " + result.error());
        }

        if (targetIndex) {
            ui::MessagePrinter::success(std::format(
                "Successfully wrote value to match #{}", *targetIndex));
        } else {
            ui::MessagePrinter::success(std::format(
                "Successfully wrote {} value(s)", result->successCount));
            if (result->failedCount > 0) {
                ui::MessagePrinter::warn(
                    std::format("{} write(s) failed", result->failedCount));
                const std::size_t errorLimit =
                    std::min<std::size_t>(result->errors.size(), 3);
                for (std::size_t i = 0; i < errorLimit; ++i) {
                    ui::MessagePrinter::warn(result->errors[i]);
                }
                if (result->errors.size() > errorLimit) {
                    ui::MessagePrinter::warn(
                        std::format("... and {} more errors",
                                    result->errors.size() - errorLimit));
                }
            }
        }

        return CommandResult{.success = true, .message = ""};
    }

   private:
    [[nodiscard]] static auto parseWriteUserValue(ScanDataType dataType,
                                                  std::string_view valueText)
        -> std::optional<UserValue> {
        std::vector<std::string> args{std::string(valueText)};
        return value::buildUserValue(dataType, ScanMatchType::MATCH_EQUAL_TO,
                                     args, 0);
    }

    SessionState* m_session;
};

}  // namespace cli::commands
