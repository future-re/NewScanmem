/**
 * @file scan.cppm
 * @brief Scan command: full/filtered workflow with optional value
 */

module;

#include <charconv>
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

namespace {

inline auto toLower(std::string str) -> std::string {
    for (auto& character : str) {
        character = static_cast<char>(
            std::tolower(static_cast<unsigned char>(character)));
    }
    return str;
}

// 支持别名与更人性化的输入（加入 int / hex）
inline auto parseDataType(std::string_view tok) -> std::optional<ScanDataType> {
    const auto TO_STR = toLower(std::string(tok));
    if (TO_STR == "any" || TO_STR == "anynumber") {
        return ScanDataType::ANYNUMBER;
    }
    if (TO_STR == "anyint" || TO_STR == "anyinteger") {
        return ScanDataType::ANYINTEGER;
    }
    if (TO_STR == "anyfloat") {
        return ScanDataType::ANYFLOAT;
    }
    if (TO_STR == "int") {  // 常见别名：默认映射为 64 位整数
        return ScanDataType::INTEGER64;
    }
    if (TO_STR == "int8" || TO_STR == "i8") {
        return ScanDataType::INTEGER8;
    }
    if (TO_STR == "int16" || TO_STR == "i16") {
        return ScanDataType::INTEGER16;
    }
    if (TO_STR == "int32" || TO_STR == "i32") {
        return ScanDataType::INTEGER32;
    }
    if (TO_STR == "int64" || TO_STR == "i64") {
        return ScanDataType::INTEGER64;
    }
    if (TO_STR == "float" || TO_STR == "float32" || TO_STR == "f32") {
        return ScanDataType::FLOAT32;
    }
    if (TO_STR == "double" || TO_STR == "float64" || TO_STR == "f64") {
        return ScanDataType::FLOAT64;
    }
    return std::nullopt;
}

inline auto parseMatchType(std::string_view tok)
    -> std::optional<ScanMatchType> {
    const auto MATCH_STR = toLower(std::string(tok));
    if (MATCH_STR == "any") {
        return ScanMatchType::MATCHANY;
    }
    if (MATCH_STR == "eq" || MATCH_STR == "=") {
        return ScanMatchType::MATCHEQUALTO;
    }
    if (MATCH_STR == "neq" || MATCH_STR == "!=") {
        return ScanMatchType::MATCHNOTEQUALTO;
    }
    if (MATCH_STR == "gt" || MATCH_STR == ">") {
        return ScanMatchType::MATCHGREATERTHAN;
    }
    if (MATCH_STR == "lt" || MATCH_STR == "<") {
        return ScanMatchType::MATCHLESSTHAN;
    }
    if (MATCH_STR == "range") {
        return ScanMatchType::MATCHRANGE;
    }
    if (MATCH_STR == "changed") {
        return ScanMatchType::MATCHCHANGED;
    }
    if (MATCH_STR == "notchanged" || MATCH_STR == "update") {
        return ScanMatchType::MATCHNOTCHANGED;
    }
    if (MATCH_STR == "inc" || MATCH_STR == "increased") {
        return ScanMatchType::MATCHINCREASED;
    }
    if (MATCH_STR == "dec" || MATCH_STR == "decreased") {
        return ScanMatchType::MATCHDECREASED;
    }
    if (MATCH_STR == "incby") {
        return ScanMatchType::MATCHINCREASEDBY;
    }
    if (MATCH_STR == "decby") {
        return ScanMatchType::MATCHDECREASEDBY;
    }
    return std::nullopt;
}

inline auto parseInt64(std::string_view str) -> std::optional<int64_t> {
    // 支持十六进制 0x 前缀
    if (str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        int64_t value{};
        const auto* begin = str.data() + 2;
        const auto* end = str.data() + str.size();
        auto [ptr, ec] = std::from_chars(begin, end, value, 16);
        if (ec == std::errc{}) {
            return value;
        }
        return std::nullopt;
    }
    int64_t value{};
    const auto* begin = str.data();
    const auto* end = str.data() + str.size();
    auto [ptr, ec] = std::from_chars(begin, end, value, 10);
    if (ec == std::errc{}) {
        return value;
    }
    return std::nullopt;
}

inline auto parseDouble(std::string_view str) -> std::optional<double> {
    try {
        return std::stod(std::string{str});
    } catch (...) {
        return std::nullopt;
    }
}

inline auto buildUserValue(ScanDataType dataType, ScanMatchType matchType,
                           const std::vector<std::string>& args,
                           size_t startIndex) -> std::optional<UserValue> {
    // Only for simple scalars; extend as needed
    if (!matchNeedsUserValue(matchType)) {
        return UserValue{};  // empty flags indicates not used
    }

    auto needRange = (matchType == ScanMatchType::MATCHRANGE);
    if (needRange && (startIndex + 1 >= args.size())) {
        return std::nullopt;
    }

    switch (dataType) {
        case ScanDataType::INTEGER8:
        case ScanDataType::INTEGER16:
        case ScanDataType::INTEGER32:
        case ScanDataType::INTEGER64:
        case ScanDataType::ANYINTEGER: {
            auto lowOpt = parseInt64(args[startIndex]);
            if (!lowOpt) {
                return std::nullopt;
            }
            if (needRange) {
                auto highOpt = parseInt64(args[startIndex + 1]);
                if (!highOpt) {
                    return std::nullopt;
                }
                return UserValue::fromRange<int64_t>(*lowOpt, *highOpt);
            }
            return UserValue::fromScalar<int64_t>(*lowOpt);
        }
        case ScanDataType::FLOAT32:
        case ScanDataType::FLOAT64:
        case ScanDataType::ANYFLOAT:
        case ScanDataType::ANYNUMBER: {
            auto lowOpt = parseDouble(args[startIndex]);
            if (!lowOpt) {
                return std::nullopt;
            }
            if (needRange) {
                auto highOpt = parseDouble(args[startIndex + 1]);
                if (!highOpt) {
                    return std::nullopt;
                }
                return UserValue::fromRange<double>(*lowOpt, *highOpt);
            }
            return UserValue::fromScalar<double>(*lowOpt);
        }
        default:
            return std::nullopt;
    }
}

}  // namespace

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
        if (!parseDataType(args[0])) {
            return std::unexpected("Unknown data type: " + args[0]);
        }
        auto matchType = parseMatchType(args[1]);
        if (!matchType) {
            return std::unexpected("Unknown match type: " + args[1]);
        }

        const bool NEEDS_VALUE = matchNeedsUserValue(*matchType);
        size_t valueCount = 0;
        if (NEEDS_VALUE) {
            if (*matchType == ScanMatchType::MATCHRANGE) {
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

        auto dataType = *parseDataType(args[0]);
        auto matchType = *parseMatchType(args[1]);

        ScanOptions opts;
        opts.dataType = dataType;
        opts.matchType = matchType;

        std::optional<UserValue> userVal;
        size_t startIdx = 2;
        if (matchNeedsUserValue(matchType)) {
            userVal = buildUserValue(dataType, matchType, args, startIdx);
            if (!userVal) {
                return std::unexpected("Invalid or missing value(s)");
            }
        }

        // 首次使用且用户选择了依赖旧值的匹配：自动做一次基线快照，再执行过滤
        bool firstPass = !scanner->hasMatches();
        bool wantsBaseline = firstPass && matchType != ScanMatchType::MATCHANY;
        if (wantsBaseline) {
            // 任何首次非 MATCHANY
            // 的调用先做全量基线，再执行过滤，保证后续基于旧值的匹配语义完整
            ScanOptions baselineOpts = opts;
            baselineOpts.matchType = ScanMatchType::MATCHANY;
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
        return CommandResult{.success = true, .message = ""};
    }

   private:
    SessionState* m_session;
};

}  // namespace cli::commands
