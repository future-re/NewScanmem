#include "memseek/cli/commands/snapshot.hpp"

namespace cli::commands {
SnapshotCommand::SnapshotCommand(SessionState& session) : m_session(&session) {}
auto SnapshotCommand::getName() const -> std::string_view { return "snapshot"; }
auto SnapshotCommand::getDescription() const -> std::string_view {
    return "Create baseline memory snapshot for later comparison";
}
auto SnapshotCommand::getUsage() const -> std::string_view {
    return "snapshot [type]\n  type (可选): "
           "int|int8|int16|int32|int64|float|double|any (默认: any)\n  示例: "
           "snapshot / snapshot int64";
}
auto SnapshotCommand::validateArgs(const std::vector<std::string>& arguments)
    const -> std::expected<void, std::string> {
    return arguments.size() <= 1 ? std::expected<void, std::string>{}
                                 : std::unexpected("Usage: snapshot [type]");
}
auto SnapshotCommand::execute(const std::vector<std::string>& arguments)
    -> std::expected<CommandResult, std::string> {
    if (!m_session || m_session->pid <= 0)
        return std::unexpected("Set target pid first: pid <pid>");
    auto scanner = m_session->ensureScanner();
    if (!scanner) return std::unexpected("Failed to initialize scanner");
    auto type = ScanDataType::ANY_NUMBER;
    if (!arguments.empty()) {
        const auto parsed = value::parseDataType(arguments.front());
        if (!parsed)
            return std::unexpected("Unknown type: " + arguments.front());
        type = *parsed;
    }
    ScanOptions options{.dataType = type,
                        .matchType = ScanMatchType::MATCH_ANY,
                        .regionLevel = m_session->regionLevel};
    const auto result = scanner->snapshot(options, std::nullopt, true);
    if (!result.success)
        return std::unexpected(result.error.value_or("Snapshot failed"));
    ui::MessagePrinter::info(
        std::format("Snapshot created: regions={}, bytes={}, matches={}",
                    result.stats.regionsVisited, result.stats.bytesScanned,
                    scanner->getMatchCount()));
    return CommandResult{.success = true, .message = {}};
}
}  // namespace cli::commands
