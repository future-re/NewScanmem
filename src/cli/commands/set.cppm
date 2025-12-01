/**
 * @file set.cppm
 * @brief Simple configuration mutation command (session & app config)
 */

module;

#include <expected>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

export module cli.commands.set;

import cli.command;
import cli.session;
import cli.app_config;
import ui.show_message;

export namespace cli::commands {

class SetCommand : public Command {
   public:
    SetCommand(cli::SessionState& session, cli::AppConfig& config)
        : m_session(&session), m_config(&config) {}

    [[nodiscard]] auto getName() const -> std::string_view override {
        return "set";
    }

    [[nodiscard]] auto getDescription() const -> std::string_view override {
        return "Set runtime options: pid|debug|color|exitOnError|init";
    }

    [[nodiscard]] auto getUsage() const -> std::string_view override {
        return "set <key> <value>\n"
               "  pid <number>        设置目标进程\n"
               "  debug on|off        是否启用调试输出\n"
               "  color on|off        是否启用彩色输出\n"
               "  exitOnError on|off  出错是否退出\n"
               "  init <commands>     初始命令(原样保存)";
    }

    [[nodiscard]] auto validateArgs(const std::vector<std::string>& args) const
        -> std::expected<void, std::string> override {
        if (args.size() < 2) {
            return std::unexpected("Usage: " + std::string(getUsage()));
        }
        return {};
    }

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override {
        if (m_session == nullptr || m_config == nullptr) {
            return std::unexpected("Internal: session/config unavailable");
        }
        const auto& key = args[0];
        if (key == "pid") {
            auto pidStr = args[1];
            auto newPid = static_cast<pid_t>(std::atoi(pidStr.c_str()));
            if (newPid <= 0) {
                return std::unexpected("Invalid pid: " + pidStr);
            }
            m_session->pid = newPid;
            m_config->targetPid = newPid;
            m_session->resetScanner();
            ui::MessagePrinter{}.info("PID set to {} (scanner reset)", newPid);
            return CommandResult{.success = true, .message = ""};
        }
        if (key == "debug") {
            auto val = args[1];
            bool debugOn = (val == "on" || val == "1" || val == "true");
            m_config->debugMode = debugOn;
            ui::MessagePrinter{}.info("Debug mode: {}", debugOn ? "ON" : "OFF");
            return CommandResult{.success = true, .message = ""};
        }
        if (key == "color") {
            auto val = args[1];
            bool colorOn = (val == "on" || val == "1" || val == "true");
            m_config->colorMode = colorOn;
            ui::MessagePrinter{}.info("Color mode: {}", colorOn ? "ON" : "OFF");
            return CommandResult{.success = true, .message = ""};
        }
        if (key == "exitOnError") {
            auto val = args[1];
            bool exitOnError = (val == "on" || val == "1" || val == "true");
            m_config->exitOnError = exitOnError;
            ui::MessagePrinter{}.info("Exit on error: {}",
                                      exitOnError ? "ON" : "OFF");
            return CommandResult{.success = true, .message = ""};
        }
        if (key == "init") {
            // 合并剩余参数作为初始命令串
            std::ostringstream oss;
            for (size_t i = 1; i < args.size(); ++i) {
                if (i > 1) {
                    oss << ' ';
                }
                oss << args[i];
            }
            m_config->initialCommands = oss.str();
            ui::MessagePrinter{}.info("Initial commands set ({} chars)",
                                      m_config->initialCommands->size());
            return CommandResult{.success = true, .message = ""};
        }
        return std::unexpected("Unknown key: " + key);
    }

   private:
    cli::SessionState* m_session;
    cli::AppConfig* m_config;
};

}  // namespace cli::commands
