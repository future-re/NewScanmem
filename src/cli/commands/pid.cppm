/**
 * @file pid.cppm
 * @brief PID command implementation for setting target process
 */

module;

#include <charconv>
#include <cstdlib>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

export module cli.commands.pid;

import cli.command;
import ui.show_message;

export namespace cli::commands {

/**
 * @class PidCommand
 * @brief Set the target process ID for memory scanning
 */
class PidCommand : public Command {
   public:
    explicit PidCommand(pid_t& targetPid) : m_targetPid(targetPid) {}

    [[nodiscard]] auto getName() const -> std::string_view override {
        return "pid";
    }

    [[nodiscard]] auto getDescription() const -> std::string_view override {
        return "Set the target process ID for memory scanning";
    }

    [[nodiscard]] auto getUsage() const -> std::string_view override {
        return "pid <process_id>";
    }

    [[nodiscard]] auto validateArgs(const std::vector<std::string>& args) const
        -> std::expected<void, std::string> override {
        if (args.empty()) {
            return std::unexpected("Missing process ID argument");
        }

        if (args.size() > 1) {
            return std::unexpected("Too many arguments");
        }

        return {};
    }

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override {
        // Parse PID
        const auto& pidStr = args[0];
        pid_t pid = 0;

        auto [ptr, ec] =
            std::from_chars(pidStr.data(), pidStr.data() + pidStr.size(), pid);

        if (ec != std::errc{}) {
            return std::unexpected("Invalid process ID: " + pidStr);
        }

        if (pid <= 0) {
            return std::unexpected("Process ID must be positive");
        }

        // Check if process exists by checking /proc/<pid>
        auto procPath = std::filesystem::path("/proc") / std::to_string(pid);
        if (!std::filesystem::exists(procPath)) {
            return std::unexpected("Process " + std::to_string(pid) +
                                   " does not exist");
        }

        // Set target process
        m_targetPid = pid;

        ui::MessagePrinter::success("Successfully set target process to " +
                                    std::to_string(pid));

        return CommandResult{.success = true, .message = ""};
    }

   private:
    // NOLINTNEXTLINE(cppcoreguidelines-avoid-const-or-ref-data-members)
    pid_t& m_targetPid;
};

}  // namespace cli::commands
