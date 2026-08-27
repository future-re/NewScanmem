#pragma once

/**
 * @file watch.hpp
 * @brief Watch command: monitor specific address for value changes
 */

#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "newscanmem/cli/command.hpp"
#include "newscanmem/cli/session.hpp"

namespace cli::commands {

class WatchCommand : public Command {
   public:
    explicit WatchCommand(SessionState& session);

    [[nodiscard]] auto getName() const -> std::string_view override;

    [[nodiscard]] auto getDescription() const -> std::string_view override;

    [[nodiscard]] auto getUsage() const -> std::string_view override;

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override;

   private:
    SessionState* m_session;
};

}  // namespace cli::commands
