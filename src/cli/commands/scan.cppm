/**
 * @file scan.cppm
 * @brief Scan command: full/filtered workflow with optional value
 */

module;

#include <expected>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

export module cli.commands.scan;

import app.result_service;
import app.scan_service;
import cli.command;
import cli.session;
import scan.types;
import value;
import value.parser;
import ui.show_message;
import utils.logging;

export namespace cli::commands {

class ScanCommand : public Command {
   public:
    explicit ScanCommand(SessionState& session) : m_session(&session) {}

    [[nodiscard]] auto getName() const -> std::string_view override {
        return "scan";
    }

    [[nodiscard]] auto getDescription() const -> std::string_view override {
        return "Memory scan (auto baseline + narrowing on subsequent calls)";
    }

    [[nodiscard]] auto getUsage() const -> std::string_view override {
        return "scan <type> <match> [value [high]]\n"
               "  <type>: "
               "int|int8|i8|int16|i16|int32|i32|int64|i64|float|double|"
               "string|str|bytearray|bytes|any|anyint|anyfloat\n"
               "  <match>: "
               "any|=|eq|!=|neq|gt|lt|range|changed|notchanged|inc|dec|incby|"
               "decby\n"
               "  示例: scan int64 = 123456 / scan int range 100 200 / scan "
               "int changed / scan string = \"Hello\"";
    }

    [[nodiscard]] auto validateArgs(const std::vector<std::string>& args) const
        -> std::expected<void, std::string> override {
        if (args.size() < 2) {
            return std::unexpected("Usage: scan <type> <match> [value [high]]");
        }
        if (!value::parseDataType(args[0])) {
            return std::unexpected("Unknown data type: " + args[0]);
        }
        auto matchType = value::parseMatchType(args[1]);
        if (!matchType) {
            return std::unexpected("Unknown match type: " + args[1]);
        }

        const bool NEEDS_VALUE = matchNeedsUserValue(*matchType);
        size_t valueCount = 0;
        if (NEEDS_VALUE) {
            if (*matchType == ScanMatchType::MATCH_RANGE) {
                valueCount = 2;
            } else {
                valueCount = 1;
            }
        }
        const size_t EXPECTED_SIZE = 2 + valueCount;
        if (args.size() < EXPECTED_SIZE) {
            return std::unexpected("Match type '" + args[1] + "' requires " +
                                   std::to_string(valueCount) + " value(s)");
        }

        if (args.size() > EXPECTED_SIZE) {
            return std::unexpected(
                "Too many arguments for scan command; expected " +
                std::to_string(EXPECTED_SIZE));
        }
        return {};
    }

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override {
        utils::Logger::info("Executing scan command...");
        if (m_session == nullptr || m_session->pid <= 0) {
            utils::Logger::error("Target PID not set.");
            return std::unexpected("Set target pid first: pid <pid>");
        }
        auto* scanner = m_session->ensureScanner();
        if (scanner == nullptr) {
            utils::Logger::error("Failed to initialize scanner.");
            return std::unexpected("Failed to initialize scanner");
        }

        auto dataType = *value::parseDataType(args[0]);
        utils::Logger::debug("Parsed DataType: {}", static_cast<int>(dataType));
        auto matchType = *value::parseMatchType(args[1]);
        utils::Logger::debug("Parsed MatchType: {}",
                             static_cast<int>(matchType));
        std::optional<UserValue> userVal;
        size_t startIdx = 2;
        if (matchNeedsUserValue(matchType)) {
            userVal =
                value::buildUserValue(dataType, matchType, args, startIdx);
        }
        if (userVal) {
            if (auto text = userVal->stringValue()) {
                utils::Logger::debug("UserValue: {}", *text);
            } else {
                utils::Logger::debug("UserValue size: {}",
                                     userVal->size());
            }
        }

        app::ScanRequest request{.scanner = scanner,
                                 .regionLevel = m_session->regionLevel,
                                 .dataType = dataType,
                                 .matchType = matchType,
                                 .userValue = userVal,
                                 .saveToHistory = true};
        auto result = app::ScanService::run(request);
        if (!result) {
            utils::Logger::error("Scan failed: {}", result.error());
            return std::unexpected(result.error());
        }

        ui::MessagePrinter::info(std::format("Current match count: {}",
                                             result->matchCount));
        utils::Logger::info("Scan command finished.");

        auto previewResult = app::ResultService::showCurrentMatches(
            {.scanner = scanner,
             .pid = m_session->pid,
             .limit = 20,
             .showRegion = true,
             .showIndex = true,
             .endianness = m_session->endianness});
        if (!previewResult) {
            return std::unexpected(previewResult.error());
        }

        return CommandResult{.success = true, .message = ""};
    }

   private:
    SessionState* m_session;
};

}  // namespace cli::commands
