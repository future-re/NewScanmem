#pragma once

/**
 * @file snapshot.hpp
 * @brief Snapshot command: manually create baseline snapshot
 */

#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "newscanmem/cli/command.hpp"
#include "newscanmem/cli/session.hpp"
#include "newscanmem/core/scanner.hpp"
#include "newscanmem/scan/types.hpp"
#include "newscanmem/ui/show_message.hpp"
#include "newscanmem/value/parser.hpp"

namespace cli::commands {

class SnapshotCommand : public Command {
   public:
    explicit SnapshotCommand(SessionState& session);

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
