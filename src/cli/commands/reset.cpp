#include "memseek/cli/commands/reset.hpp"

namespace cli::commands {
ResetCommand::ResetCommand(SessionState& session) : m_session(&session) {}
auto ResetCommand::getName() const -> std::string_view { return "reset"; }
auto ResetCommand::getDescription() const -> std::string_view {
    return "Clear current matches and history";
}
auto ResetCommand::getUsage() const -> std::string_view { return "reset"; }
auto ResetCommand::validateArgs(const std::vector<std::string>& arguments) const
    -> std::expected<void, std::string> {
    return arguments.empty() ? std::expected<void, std::string>{}
                             : std::unexpected("'reset' takes no arguments");
}
auto ResetCommand::execute(const std::vector<std::string>&)
    -> std::expected<CommandResult, std::string> {
    if (m_session && m_session->scanner) m_session->scanner->reset();
    ui::MessagePrinter::info("Scanner state reset");
    return CommandResult{.success = true, .message = {}};
}
}  // namespace cli::commands
