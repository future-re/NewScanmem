#include "newscanmem/cli/app.hpp"

#include "newscanmem/cli/command.hpp"
#include "newscanmem/cli/repl.hpp"
#include "newscanmem/cli/commands/count.hpp"
#include "newscanmem/cli/commands/help.hpp"
#include "newscanmem/cli/commands/list.hpp"
#include "newscanmem/cli/commands/pid.hpp"
#include "newscanmem/cli/commands/quit.hpp"
#include "newscanmem/cli/commands/reset.hpp"
#include "newscanmem/cli/commands/scan.hpp"
#include "newscanmem/cli/commands/set.hpp"
#include "newscanmem/cli/commands/snapshot.hpp"
#include "newscanmem/cli/commands/watch.hpp"
#include "newscanmem/cli/commands/write.hpp"
#include "newscanmem/ui/console.hpp"
#include "newscanmem/ui/show_message.hpp"
#include "newscanmem/utils/logging.hpp"

namespace cli {
Application::Application(const AppConfig& config) : m_config(config), m_ui(std::make_shared<ui::ConsoleUI>(ui::MessageContext{.debugMode = config.debugMode, .backendMode = config.backendMode, .colorMode = config.colorMode})) {}
auto Application::run() -> int { registerCommands(); utils::Logger::instance().init("/log/scanmem.log", utils::LogLevel::DEBUG); if (m_session.pid != 0) ui::MessagePrinter{}.info("Target PID: {}", m_session.pid); else if (m_config.targetPid != 0) { m_session.pid = m_config.targetPid; m_session.regionLevel = m_config.regionLevel; ui::MessagePrinter{}.info("Target PID: {}", m_config.targetPid); } else m_ui->printInfo("Enter the pid of the process using the 'pid' command."); m_ui->printInfo("Type 'help' for available commands."); if (const auto console = dynamic_cast<ui::ConsoleUI*>(m_ui.get())) console->setCompletionCallback([](const std::string_view prefix) { return getCommandCompletions(prefix); }); if (m_config.initialCommands && (executeCommandString(*m_config.initialCommands) || m_config.batchMode)) return 0; if (m_config.batchMode) return 0; return REPL{m_ui, buildPrompt(), [this] { return buildPrompt(); }}.run(); }
auto Application::executeCommandString(const std::string& commands) -> bool { std::size_t start = 0; while (start <= commands.size()) { const auto end = commands.find(';', start); const auto command = commands.substr(start, end - start); if (!command.empty()) { const auto result = REPL::executeLine(command); if (result && result->shouldExit) return true; } if (end == std::string::npos) break; start = end + 1; } return false; }
auto Application::getCommandCompletions(const std::string_view prefix) -> std::vector<std::string> { std::vector<std::string> candidates; for (const auto command : CommandRegistry::instance().getAllCommands()) { const auto name = std::string(command->getName()); if (name.starts_with(prefix)) candidates.push_back(name); for (const auto alias : command->getAliases()) if (std::string_view(alias).starts_with(prefix)) candidates.emplace_back(alias); } return candidates; }
void Application::registerCommands() { auto& registry = CommandRegistry::instance(); registry.clear(); registry.registerCommand(std::make_unique<commands::HelpCommand>()); registry.registerCommand(std::make_unique<commands::QuitCommand>()); registry.registerCommand(std::make_unique<commands::PidCommand>(m_session)); registry.registerCommand(std::make_unique<commands::ScanCommand>(m_session)); registry.registerCommand(std::make_unique<commands::ResetCommand>(m_session)); registry.registerCommand(std::make_unique<commands::CountCommand>(m_session)); registry.registerCommand(std::make_unique<commands::SetCommand>(m_session, m_config)); registry.registerCommand(std::make_unique<commands::SnapshotCommand>(m_session)); registry.registerCommand(std::make_unique<commands::ListCommand>(m_session)); registry.registerCommand(std::make_unique<commands::WriteCommand>(m_session)); registry.registerCommand(std::make_unique<commands::WatchCommand>(m_session)); }
auto Application::buildPrompt() const -> std::string { return m_session.pid > 0 ? std::to_string(m_session.pid) + "> " : "> "; }
}  // namespace cli
