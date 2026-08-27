#include "newscanmem/cli/commands/pid.hpp"

namespace cli::commands {
PidCommand::PidCommand(SessionState& session) : m_session(&session) {}
auto PidCommand::getName() const -> std::string_view { return "pid"; }
auto PidCommand::getDescription() const -> std::string_view { return "Set the target process ID for memory scanning"; }
auto PidCommand::getUsage() const -> std::string_view { return "pid <process_id>"; }
auto PidCommand::validateArgs(const std::vector<std::string>& arguments) const -> std::expected<void, std::string> { if (arguments.empty()) return std::unexpected("Missing process ID argument"); return arguments.size() == 1 ? std::expected<void, std::string>{} : std::unexpected("Too many arguments"); }
auto PidCommand::execute(const std::vector<std::string>& arguments) -> std::expected<CommandResult, std::string> { pid_t pid{}; const auto& text = arguments.front(); const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), pid); if (error != std::errc{} || end != text.data() + text.size()) return std::unexpected("Invalid process ID: " + text); if (pid <= 0) return std::unexpected("Process ID must be positive"); if (!std::filesystem::exists(std::filesystem::path{"/proc"} / std::to_string(pid))) return std::unexpected("Process " + std::to_string(pid) + " does not exist"); if (!m_session) return std::unexpected("Session state unavailable"); m_session->pid = pid; m_session->scanner.reset(); ui::MessagePrinter::success("Successfully set target process to " + std::to_string(pid)); return CommandResult{.success = true, .message = {}}; }
}  // namespace cli::commands
