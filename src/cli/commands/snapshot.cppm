/**
 * @file snapshot.cppm
 * @brief Snapshot command: manually create baseline snapshot
 */

module;

#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <vector>

export module cli.commands.snapshot;

import cli.command;
import cli.session;
import ui.show_message;
import core.scanner;
import scan.engine;
import scan.types;

export namespace cli::commands {

class SnapshotCommand : public Command {
   public:
    explicit SnapshotCommand(SessionState& session) : m_session(&session) {}

    [[nodiscard]] auto getName() const -> std::string_view override {
        return "snapshot";
    }

    [[nodiscard]] auto getDescription() const -> std::string_view override {
        return "Create baseline memory snapshot for later comparison";
    }

    [[nodiscard]] auto getUsage() const -> std::string_view override {
        return "snapshot [type]\n"
               "  type (可选): int|int8|int16|int32|int64|float|double|any "
               "(默认: any)\n"
               "  示例: snapshot / snapshot int64";
    }

    [[nodiscard]] auto validateArgs(const std::vector<std::string>& args) const
        -> std::expected<void, std::string> override {
        if (args.size() > 1) {
            return std::unexpected("Usage: snapshot [type]");
        }
        return {};
    }

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override {
        if (m_session == nullptr || m_session->pid <= 0) {
            return std::unexpected("Set target pid first: pid <pid>");
        }
        auto* scanner = m_session->ensureScanner();
        if (scanner == nullptr) {
            return std::unexpected("Failed to initialize scanner");
        }

        // 默认使用 ANYNUMBER 类型
        ScanDataType dataType = ScanDataType::ANY_NUMBER;
        if (!args.empty()) {
            auto typeStr = args[0];
            // 简化的类型解析
            if (typeStr == "int" || typeStr == "int64" || typeStr == "i64") {
                dataType = ScanDataType::INTEGER_64;
            } else if (typeStr == "int32" || typeStr == "i32") {
                dataType = ScanDataType::INTEGER_32;
            } else if (typeStr == "int16" || typeStr == "i16") {
                dataType = ScanDataType::INTEGER_16;
            } else if (typeStr == "int8" || typeStr == "i8") {
                dataType = ScanDataType::INTEGER_8;
            } else if (typeStr == "float" || typeStr == "FLOAT_32" ||
                       typeStr == "f32") {
                dataType = ScanDataType::FLOAT_32;
            } else if (typeStr == "double" || typeStr == "FLOAT_64" ||
                       typeStr == "f64") {
                dataType = ScanDataType::FLOAT_64;
            } else if (typeStr == "any") {
                dataType = ScanDataType::ANY_NUMBER;
            } else {
                return std::unexpected("Unknown type: " + typeStr);
            }
        }

        ScanOptions opts;
        opts.dataType = dataType;
        opts.matchType = ScanMatchType::MATCH_ANY;

        auto res = scanner->performScan(opts, nullptr, true);
        if (!res) {
            return std::unexpected(res.error());
        }

        ui::MessagePrinter::info(std::format(
            "Snapshot created: regions={}, bytes={}, matches={}",
            res->regionsVisited, res->bytesScanned, scanner->getMatchCount()));

        return CommandResult{.success = true, .message = ""};
    }

   private:
    SessionState* m_session;
};

}  // namespace cli::commands
