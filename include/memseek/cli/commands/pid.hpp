#pragma once

/**
 * @file pid.hpp
 * @brief PID command implementation for setting target process
 */

#include <cstdlib>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "memseek/cli/command.hpp"
#include "memseek/cli/session.hpp"

namespace cli::commands {

/**
 * @class PidCommand
 * @brief Set the target process ID for memory scanning
 */
class PidCommand : public Command {
   public:
    explicit PidCommand(SessionState& session);

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
