#include "newscanmem/cli/commands/set.hpp"

#include <charconv>
#include <sstream>

#include "newscanmem/core/maps.hpp"
#include "newscanmem/ui/show_message.hpp"

namespace cli::commands {

SetCommand::SetCommand(SessionState& session, AppConfig& config)
    : m_session(&session), m_config(&config) {}
auto SetCommand::getName() const -> std::string_view { return "set"; }
auto SetCommand::getDescription() const -> std::string_view {
    return "Set runtime options: pid|debug|color|autoBaseline|exitOnError|init";
}
auto SetCommand::getUsage() const -> std::string_view {
    return "set <key> <value>\n"
           "  pid <number>         设置目标进程\n"
           "  debug on|off         是否启用调试输出\n"
           "  color on|off         是否启用彩色输出\n"
           "  autoBaseline on|off  首次扫描时自动建立基线\n"
           "  exitOnError on|off   出错是否退出\n"
           "  regionLevel ALL|ALL_RW|HEAP_STACK_EXECUTABLE|HEAP_STACK_EXECUTABLE_BSS 设置扫描区域\n"
           "  init <commands>      初始命令(原样保存)";
}
auto SetCommand::validateArgs(const std::vector<std::string>& args) const
    -> std::expected<void, std::string> {
    if (args.size() < 2) return std::unexpected("Usage: " + std::string(getUsage()));
    return {};
}
auto SetCommand::execute(const std::vector<std::string>& args)
    -> std::expected<CommandResult, std::string> {
    if (m_session == nullptr || m_config == nullptr)
        return std::unexpected("Internal: session/config unavailable");
    const auto& key = args[0];
    auto booleanOption = [](const std::string& value, bool& field,
                            const std::string& name) {
        const bool enabled = value == "on" || value == "1" || value == "true";
        field = enabled;
        ui::MessagePrinter{}.info("{}: {}", name, enabled ? "ON" : "OFF");
        return CommandResult{.success = true, .message = ""};
    };
    if (key == "pid") {
        const auto& pidString = args[1];
        pid_t newPid = 0;
        const auto [end, ec] = std::from_chars(pidString.data(), pidString.data() + pidString.size(), newPid);
        if (ec != std::errc{} || end != pidString.data() + pidString.size() || newPid <= 0)
            return std::unexpected("Invalid pid: " + pidString);
        m_session->pid = newPid;
        m_config->targetPid = newPid;
        m_session->resetScanner();
        ui::MessagePrinter{}.info("PID set to {} (scanner reset)", newPid);
        return CommandResult{.success = true, .message = ""};
    }
    if (key == "debug") return booleanOption(args[1], m_config->debugMode, "Debug mode");
    if (key == "color") return booleanOption(args[1], m_config->colorMode, "Color mode");
    if (key == "autoBaseline") return booleanOption(args[1], m_config->autoBaseline, "Auto baseline");
    if (key == "exitOnError") return booleanOption(args[1], m_config->exitOnError, "Exit on error");
    if (key == "regionLevel") {
        const auto& value = args[1];
        core::RegionScanLevel level;
        if (value == "ALL") level = core::RegionScanLevel::ALL;
        else if (value == "ALL_RW") level = core::RegionScanLevel::ALL_RW;
        else if (value == "HEAP_STACK_EXECUTABLE") level = core::RegionScanLevel::HEAP_STACK_EXECUTABLE;
        else if (value == "HEAP_STACK_EXECUTABLE_BSS") level = core::RegionScanLevel::HEAP_STACK_EXECUTABLE_BSS;
        else return std::unexpected("Invalid regionLevel: " + value + ". Valid values: ALL, ALL_RW, HEAP_STACK_EXECUTABLE, HEAP_STACK_EXECUTABLE_BSS");
        m_config->regionLevel = level;
        m_session->regionLevel = level;
        ui::MessagePrinter{}.info("Region level: {}", value);
        return CommandResult{.success = true, .message = ""};
    }
    if (key == "init") {
        std::ostringstream oss;
        for (size_t i = 1; i < args.size(); ++i) {
            if (i > 1) oss << ' ';
            oss << args[i];
        }
        m_config->initialCommands = oss.str();
        ui::MessagePrinter{}.info("Initial commands set ({} chars)", m_config->initialCommands->size());
        return CommandResult{.success = true, .message = ""};
    }
    return std::unexpected("Unknown key: " + key);
}

}  // namespace cli::commands
