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
import core.maps;

// 确保显式使用 cli 命名空间
using cli::AppConfig;

export namespace cli::commands {

class SetCommand : public Command {
   public:
    SetCommand(cli::SessionState& session, AppConfig& config)
        : m_session(&session), m_config(&config) {}

    [[nodiscard]] auto getName() const -> std::string_view override {
        return "set";
    }

    [[nodiscard]] auto getDescription() const -> std::string_view override {
        return "Set runtime options: "
               "pid|debug|color|autoBaseline|exitOnError|init";
    }

    [[nodiscard]] auto getUsage() const -> std::string_view override {
        return "set <key> <value>\n"
               "  pid <number>         设置目标进程\n"
               "  debug on|off         是否启用调试输出\n"
               "  color on|off         是否启用彩色输出\n"
               "  autoBaseline on|off  首次扫描时自动建立基线\n"
               "  exitOnError on|off   出错是否退出\n"
               "  regionLevel "
               "ALL|ALL_RW|HEAP_STACK_EXECUTABLE|HEAP_STACK_EXECUTABLE_BSS "
               "设置扫描区域\n"
               "  init <commands>      初始命令(原样保存)";
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

        // Lambda to handle boolean configuration options
        auto handleBooleanOption =
            [&](const std::string& value, bool& configField,
                const std::string& optionName) -> CommandResult {
            bool isEnabled = (value == "on" || value == "1" || value == "true");
            configField = isEnabled;
            ui::MessagePrinter{}.info("{}: {}", optionName,
                                      isEnabled ? "ON" : "OFF");
            return CommandResult{.success = true, .message = ""};
        };

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
            return handleBooleanOption(args[1], m_config->debugMode,
                                       "Debug mode");
        }
        if (key == "color") {
            return handleBooleanOption(args[1], m_config->colorMode,
                                       "Color mode");
        }
        if (key == "autoBaseline") {
            return handleBooleanOption(args[1], m_config->autoBaseline,
                                       "Auto baseline");
        }
        if (key == "exitOnError") {
            return handleBooleanOption(args[1], m_config->exitOnError,
                                       "Exit on error");
        }
        if (key == "regionLevel") {
            const auto& levelStr = args[1];
            core::RegionScanLevel level = core::RegionScanLevel::ALL_RW;
            if (levelStr == "ALL") {
                level = core::RegionScanLevel::ALL;
            } else if (levelStr == "ALL_RW") {
                level = core::RegionScanLevel::ALL_RW;
            } else if (levelStr == "HEAP_STACK_EXECUTABLE") {
                level = core::RegionScanLevel::HEAP_STACK_EXECUTABLE;
            } else if (levelStr == "HEAP_STACK_EXECUTABLE_BSS") {
                level = core::RegionScanLevel::HEAP_STACK_EXECUTABLE_BSS;
            } else {
                return std::unexpected(
                    "Invalid regionLevel: " + levelStr +
                    ". Valid values: ALL, ALL_RW, HEAP_STACK_EXECUTABLE, "
                    "HEAP_STACK_EXECUTABLE_BSS");
            }
            m_config->regionLevel = level;
            ui::MessagePrinter{}.info("Region level: {}", levelStr);
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
    AppConfig* m_config;
};

}  // namespace cli::commands
