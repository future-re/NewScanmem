#include "newscanmem/cli/repl.hpp"

namespace cli {
REPL::REPL(std::shared_ptr<ui::UserInterface> interface, std::string prompt, std::function<std::string()> builder) : m_ui(std::move(interface)), m_prompt(std::move(prompt)), m_promptBuilder(std::move(builder)) {}
auto REPL::run() -> int { if (!m_ui) { ui::MessagePrinter::error("No user interface provided"); return 1; } ui::MessagePrinter::info("NewScanmem Interactive Console"); ui::MessagePrinter::info("Type 'help' for available commands, 'quit' to exit\n"); for (;;) { const auto prompt = m_promptBuilder ? m_promptBuilder() : m_prompt; const auto line = m_ui->getLine(prompt); if (!line) break; if (line->empty()) continue; const auto result = executeLine(*line); if (!result) { ui::MessagePrinter::error("Error: " + result.error()); continue; } if (result->shouldExit) return 0; if (!result->message.empty()) result->success ? ui::MessagePrinter::info(result->message) : ui::MessagePrinter::warn(result->message); } return 0; }
auto REPL::executeLine(const std::string& line) -> std::expected<CommandResult, std::string> { auto [name, args] = parseCommandLine(line); return name.empty() ? std::expected<CommandResult, std::string>{CommandResult{.success = true}} : executeCommand(name, args); }
void REPL::setPrompt(const std::string_view prompt) { m_prompt = prompt; }
void REPL::setPromptBuilder(std::function<std::string()> builder) { m_promptBuilder = std::move(builder); }
auto REPL::getPrompt() const -> std::string_view { return m_prompt; }
auto REPL::executeCommand(const std::string& name, const std::vector<std::string>& arguments) -> std::expected<CommandResult, std::string> { const auto command = CommandRegistry::instance().findCommand(name); if (!command) return std::unexpected("Unknown command: " + name + ". Type 'help' for available commands."); const auto valid = command->validateArgs(arguments); if (!valid) return std::unexpected(valid.error() + "\nUsage: " + std::string(command->getUsage())); return command->execute(arguments); }
}  // namespace cli
