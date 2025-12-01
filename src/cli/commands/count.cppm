/**
 * @file count.cppm
 * @brief Count command: show current match count
 */

module;

#include <expected>
#include <string>
#include <string_view>
#include <vector>

export module cli.commands.count;

import cli.command;
import cli.session;
import ui.show_message;

export namespace cli::commands {

class CountCommand : public Command {
   public:
    explicit CountCommand(SessionState& session) : m_session(&session) {}

    [[nodiscard]] auto getName() const -> std::string_view override {
        return "count";
    }

    [[nodiscard]] auto getDescription() const -> std::string_view override {
        return "Show number of current matches";
    }

    [[nodiscard]] auto getUsage() const -> std::string_view override {
        return "count";
    }

    [[nodiscard]] auto validateArgs(const std::vector<std::string>& args) const
        -> std::expected<void, std::string> override {
        if (!args.empty()) {
            return std::unexpected("'count' takes no arguments");
        }
        return {};
    }

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override {
        (void)args;
        if (m_session == nullptr || !m_session->scanner) {
            ui::MessagePrinter::info("Current match count: 0");
            return CommandResult{.success = true, .message = ""};
        }
        auto count = m_session->scanner->getMatchCount();
        ui::MessagePrinter{}.info("Current match count: {}", count);
        return CommandResult{.success = true, .message = ""};
    }

   private:
    SessionState* m_session;
};

}  // namespace cli::commands
