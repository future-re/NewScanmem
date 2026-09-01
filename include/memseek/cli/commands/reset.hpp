#pragma once

/**
 * @file reset.hpp
 * @brief Reset command: clear matches and history
 */

#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "memseek/cli/command.hpp"
#include "memseek/cli/session.hpp"
#include "memseek/ui/show_message.hpp"

namespace cli::commands {

class ResetCommand : public Command {
   public:
    explicit ResetCommand(SessionState& session);

    [[nodiscard]] auto getName() const -> std::string_view override;

    [[nodiscard]] auto getDescription() const -> std::string_view override;

    [[nodiscard]] auto getUsage() const -> std::string_view override;

    [[nodiscard]] auto validateArgs(const std::vector<std::string>& args) const
        -> std::expected<void, std::string> override;

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override;

   private:
    SessionState* m_session;
};

}  // namespace cli::commands
