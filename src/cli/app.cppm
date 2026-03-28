/**
 * @file cli_app.cppm
 * @brief Main CLI application orchestrator
 */

module;

#include <memory>
#include <string>
#include <string_view>
#include <vector>

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
import utils.logging;

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
                                 .backendMode = config.backendMode,
                                 .colorMode = config.colorMode})) {}

    /**
     * @brief Initialize and run the application
     * @return Exit code
     */
    auto run() -> int {
        registerCommands();
        utils::Logger::instance().init("/log/scanmem.log",
                                       utils::LogLevel::DEBUG);

        if (m_session.pid != 0) {
            ui::MessagePrinter{}.info("Target PID: {}", m_session.pid);
        } else if (m_config.targetPid != 0) {
            m_session.pid = m_config.targetPid;
            ui::MessagePrinter{}.info("Target PID: {}", m_config.targetPid);
        } else {
            m_ui->printInfo(
                "Enter the pid of the process using the 'pid' command.");
        }

        m_ui->printInfo("Type 'help' for available commands.");

        // 设置命令补全回调
        if (auto* consoleUI = dynamic_cast<ui::ConsoleUI*>(m_ui.get())) {
            consoleUI->setCompletionCallback(
                [](std::string_view prefix) -> std::vector<std::string> {
                    return getCommandCompletions(prefix);
                });
        }

        // Execute initial commands if present
        if (m_config.initialCommands) {
            bool shouldExit = executeCommandString(*m_config.initialCommands);
            if (shouldExit || m_config.batchMode) {
                return 0;
            }
        }

        if (m_config.batchMode) {
            return 0;
        }

        // Start interactive REPL
        REPL repl{m_ui, buildPrompt(), [this]() { return buildPrompt(); }};
        return repl.run();
    }

   private:
    /**
     * @brief Execute a string containing multiple commands separated by
     * semicolons
     * @param commands Semicolon-separated commands
     * @return True if any command requested exit
     */
    auto executeCommandString(const std::string& commands) -> bool {
        std::string command;
        std::string_view sv = commands;
        size_t start = 0;
        size_t end;

        while ((end = sv.find(';', start)) != std::string_view::npos) {
            command = std::string(sv.substr(start, end - start));
            if (!command.empty()) {
                auto result = REPL::executeLine(command);
                if (result && result->shouldExit) {
                    return true;
                }
            }
            start = end + 1;
        }

        command = std::string(sv.substr(start));
        if (!command.empty()) {
            auto result = REPL::executeLine(command);
            if (result && result->shouldExit) {
                return true;
            }
        }

        return false;
    }
    /**
     * @brief Get command completions for a given prefix
     * @param prefix Input prefix to complete
     * @return Vector of matching command names
     */
    static auto getCommandCompletions(std::string_view prefix)
        -> std::vector<std::string> {
        auto& registry = CommandRegistry::instance();
        auto allCommands = registry.getAllCommands();

        std::vector<std::string> candidates;

        for (auto* cmd : allCommands) {
            std::string cmdName{cmd->getName()};

            // 检查命令名是否匹配前缀
            if (cmdName.starts_with(prefix)) {
                candidates.push_back(cmdName);
            }

            // 检查别名
            for (auto alias : cmd->getAliases()) {
                std::string aliasStr{alias};
                if (aliasStr.starts_with(prefix)) {
                    candidates.push_back(aliasStr);
                }
            }
        }

        return candidates;
    }

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
