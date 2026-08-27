#pragma once

/**
 * @file scan.hpp
 * @brief Scan command: full/filtered workflow with optional value
 */

#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "newscanmem/cli/command.hpp"
#include "newscanmem/cli/session.hpp"

namespace cli::commands {

class ScanCommand : public Command {
   public:
    explicit ScanCommand(SessionState& session);

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
