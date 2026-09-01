#pragma once

/**
 * @file list.hpp
 * @brief List command: display current matches with addresses and values
 */

#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "memseek/cli/command.hpp"
#include "memseek/cli/session.hpp"

namespace cli::commands {

class ListCommand : public Command {
   public:
    explicit ListCommand(SessionState& session);

    [[nodiscard]] auto getName() const -> std::string_view override;

    [[nodiscard]] auto getDescription() const -> std::string_view override;

    [[nodiscard]] auto getUsage() const -> std::string_view override;

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override;

   private:
    SessionState* m_session;
};

}  // namespace cli::commands
