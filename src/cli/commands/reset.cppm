/**
 * @file reset.cppm
 * @brief Reset command: clear matches and history
 */

module;

#include <expected>
#include <string>
#include <string_view>
#include <vector>

export module cli.commands.reset;

import cli.command;
import cli.session;
import ui.show_message;

export namespace cli::commands {

class ResetCommand : public Command {
   public:
    explicit ResetCommand(SessionState& session) : m_session(&session) {}

    [[nodiscard]] auto getName() const -> std::string_view override {
        return "reset";
    }

    [[nodiscard]] auto getDescription() const -> std::string_view override {
        return "Clear current matches and history";
    }

    [[nodiscard]] auto getUsage() const -> std::string_view override {
        return "reset";
    }

    [[nodiscard]] auto validateArgs(const std::vector<std::string>& args) const
        -> std::expected<void, std::string> override {
        if (!args.empty()) {
            return std::unexpected("'reset' takes no arguments");
        }
        return {};
    }

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override {
        (void)args;
        if (m_session != nullptr && m_session->scanner) {
            m_session->scanner->reset();
        }
        ui::MessagePrinter::info("Scanner state reset");
        return CommandResult{.success = true, .message = ""};
    }

   private:
    SessionState* m_session;
};

}  // namespace cli::commands
