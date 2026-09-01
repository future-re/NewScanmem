#include "memseek/cli/commands/write.hpp"

#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "memseek/core/memory_writer.hpp"
#include "memseek/scan/types.hpp"
#include "memseek/ui/show_message.hpp"
#include "memseek/value/core.hpp"
#include "memseek/value/parser.hpp"

namespace cli::commands {

WriteCommand::WriteCommand(SessionState& session) : m_session(&session) {}

auto WriteCommand::getName() const -> std::string_view { return "write"; }

auto WriteCommand::getDescription() const -> std::string_view {
    return "Write value to memory at matched addresses";
}

auto WriteCommand::getUsage() const -> std::string_view {
    return "write <value> [index]\n"
           "  value: 要写入的值 (支持十六进制 0x...)\n"
           "  index (可选): 匹配索引 (默认: 写入所有匹配)\n"
           "  示例: write 100 / write 0xff 0";
}

auto WriteCommand::validateArgs(const std::vector<std::string>& args) const
    -> std::expected<void, std::string> {
    if (args.empty() || args.size() > 2) {
        return std::unexpected("Usage: write <value> [index]");
    }
    return {};
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto WriteCommand::execute(const std::vector<std::string>& args)
    -> std::expected<CommandResult, std::string> {
    if (m_session == nullptr || m_session->pid <= 0) {
        return std::unexpected("Set target pid first: pid <pid>");
    }
    if (!m_session->scanner) {
        return std::unexpected("No matches. Run a scan first.");
    }

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
        ui::MessagePrinter::success(
            std::format("Successfully wrote value to match #{}", *targetIndex));
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

auto WriteCommand::parseWriteUserValue(ScanDataType dataType,
                                       std::string_view valueText)
    -> std::optional<UserValue> {
    std::vector<std::string> args{std::string(valueText)};
    return value::buildUserValue(dataType, ScanMatchType::MATCH_EQUAL_TO, args,
                                 0);
}

}  // namespace cli::commands
