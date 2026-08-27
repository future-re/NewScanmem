#include "newscanmem/cli/commands/count.hpp"

namespace cli::commands {
CountCommand::CountCommand(SessionState& session) : m_session(&session) {}
auto CountCommand::getName() const -> std::string_view { return "count"; }
auto CountCommand::getDescription() const -> std::string_view {
    return "Show number of current matches";
}
auto CountCommand::getUsage() const -> std::string_view { return "count"; }
auto CountCommand::validateArgs(const std::vector<std::string>& arguments) const
    -> std::expected<void, std::string> {
    return arguments.empty() ? std::expected<void, std::string>{}
                             : std::unexpected("'count' takes no arguments");
}
auto CountCommand::execute(const std::vector<std::string>&)
    -> std::expected<CommandResult, std::string> {
    const auto count = m_session && m_session->scanner
                           ? m_session->scanner->getMatchCount()
                           : 0U;
    ui::MessagePrinter{}.info("Current match count: {}", count);
    return CommandResult{.success = true, .message = {}};
}
}  // namespace cli::commands
