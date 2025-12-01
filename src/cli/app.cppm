/**
 * @file cli_app.cppm
 * @brief Main CLI application orchestrator
 */

module;

#include <memory>
#include <string>
#include <string_view>

export module cli.app;

import cli.app_config;
import cli.session;
import cli.command;
import cli.repl;
import cli.commands.help;
import cli.commands.pid;
import cli.commands.quit;
import cli.commands.scan;
import cli.commands.reset;
import cli.commands.count;
import cli.commands.set;
import cli.commands.snapshot;
import cli.commands.list;
import cli.commands.write;
import cli.commands.watch;
import ui.interface;
import ui.console;
import ui.show_message;

export namespace cli {

/**
 * @class Application
 * @brief Main application controller: init, command registry, REPL run
 */
class Application {
   public:
    explicit Application(const AppConfig& config)
        : m_config(config),
          m_ui(std::make_shared<ui::ConsoleUI>(
              ui::MessageContext{.debugMode = config.debugMode,
                                 .backendMode = false,
                                 .colorMode = config.colorMode})) {}

    /**
     * @brief Initialize and run the application
     * @return Exit code
     */
    auto run() -> int {
        registerCommands();

        if (m_config.targetPid != 0) {
            m_session.pid = m_config.targetPid;
            ui::MessagePrinter{}.info("Target PID: {}", m_config.targetPid);
        } else {
            m_ui->printInfo(
                "Enter the pid of the process using the 'pid' command.");
        }

        m_ui->printInfo("Type 'help' for available commands.");

        // Execute initial commands if present
        if (m_config.initialCommands) {
            // TODO: execute initial commands via REPL or direct dispatch
        }

        // Start interactive REPL (prompt reflects current pid)
        REPL repl{m_ui, buildPrompt(), [this]() { return buildPrompt(); }};
        return repl.run();
    }

   private:
    auto registerCommands() -> void {
        auto& registry = CommandRegistry::instance();
        registry.clear();

        registry.registerCommand(std::make_unique<commands::HelpCommand>());
        registry.registerCommand(std::make_unique<commands::QuitCommand>());
        registry.registerCommand(
            std::make_unique<commands::PidCommand>(m_session));
        registry.registerCommand(
            std::make_unique<commands::ScanCommand>(m_session));
        registry.registerCommand(
            std::make_unique<commands::ResetCommand>(m_session));
        registry.registerCommand(
            std::make_unique<commands::CountCommand>(m_session));
        registry.registerCommand(
            std::make_unique<commands::SetCommand>(m_session, m_config));
        registry.registerCommand(
            std::make_unique<commands::SnapshotCommand>(m_session));
        registry.registerCommand(
            std::make_unique<commands::ListCommand>(m_session));
        registry.registerCommand(
            std::make_unique<commands::WriteCommand>(m_session));
        registry.registerCommand(
            std::make_unique<commands::WatchCommand>(m_session));
    }

    auto buildPrompt() const -> std::string {
        if (m_session.pid > 0) {
            return std::to_string(m_session.pid) + "> ";
        }
        return "> ";
    }

    AppConfig m_config;
    SessionState m_session;
    std::shared_ptr<ui::UserInterface> m_ui;
};

}  // namespace cli
