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

import cli.command;
import cli.session;
import ui.show_message;
import core.scanner;
import scan.engine;
import scan.types;
import value;
import value.parser;
import cli.commands.list;

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
               "int|int8|i8|int16|i16|int32|i32|int64|i64|float|double|any|"
               "anyint|anyfloat\n"
               "  <match>: "
               "any|=|eq|!=|neq|gt|lt|range|changed|notchanged|inc|dec|incby|"
               "decby\n"
               "  示例: scan int64 = 123456 / scan int range 100 200 / scan "
               "int changed";
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
        if (m_session == nullptr || m_session->pid <= 0) {
            return std::unexpected("Set target pid first: pid <pid>");
        }
        auto* scanner = m_session->ensureScanner();
        if (scanner == nullptr) {
            return std::unexpected("Failed to initialize scanner");
        }

        auto dataType = *value::parseDataType(args[0]);
        auto matchType = *value::parseMatchType(args[1]);

        ScanOptions opts;
        opts.dataType = dataType;
        opts.matchType = matchType;

        std::optional<UserValue> userVal;
        size_t startIdx = 2;
        if (matchNeedsUserValue(matchType)) {
            userVal =
                value::buildUserValue(dataType, matchType, args, startIdx);
            if (!userVal) {
                return std::unexpected("Invalid or missing value(s)");
            }
        }

        // 首次使用且用户选择了依赖旧值的匹配：自动做一次基线快照，再执行过滤
        bool firstPass = !scanner->hasMatches();
        bool wantsBaseline = firstPass && matchType != ScanMatchType::MATCH_ANY;
        if (wantsBaseline) {
            // 任何首次非 MATCH_ANY
            // 的调用先做全量基线，再执行过滤，保证后续基于旧值的匹配语义完整
            ScanOptions baselineOpts = opts;
            baselineOpts.matchType = ScanMatchType::MATCH_ANY;
            auto baseRes = scanner->performScan(baselineOpts, nullptr, true);
            if (!baseRes) {
                return std::unexpected(baseRes.error());
            }
            auto filteredRes = scanner->performFilteredScan(
                opts, userVal ? &*userVal : nullptr, true);
            if (!filteredRes) {
                return std::unexpected(filteredRes.error());
            }
            ui::MessagePrinter::info("Baseline snapshot created");
            ui::MessagePrinter::info(std::format("regions={} bytes={}",
                                                 baseRes->regionsVisited,
                                                 baseRes->bytesScanned));
            ui::MessagePrinter::info(std::format(
                "Filtered applied (matches={})", scanner->getMatchCount()));
        } else {
            auto res = scanner->scan(opts, userVal ? &*userVal : nullptr, true);
            if (!res) {
                return std::unexpected(res.error());
            }
            ui::MessagePrinter::info(
                std::format("Scan completed: regions={}, bytes={} (matches={})",
                            res->regionsVisited, res->bytesScanned,
                            scanner->getMatchCount()));
        }
        ui::MessagePrinter::info(
            std::format("Current match count: {}", scanner->getMatchCount()));
        ListCommand listCmd(*m_session);
        (void)listCmd.execute({});
        return CommandResult{.success = true, .message = ""};
    }

   private:
    SessionState* m_session;
};

}  // namespace cli::commands
