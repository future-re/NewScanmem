#pragma once

/**
 * @file set.hpp
 * @brief Simple configuration mutation command (session & app config)
 */

#include <expected>
#include <string>
#include <string_view>
#include <vector>

#include "newscanmem/cli/command.hpp"
#include "newscanmem/cli/session.hpp"

#include "newscanmem/cli/app_config.hpp"

// 确保显式使用 cli 命名空间
using cli::AppConfig;

namespace cli::commands {

class SetCommand : public Command {
   public:
    SetCommand(cli::SessionState& session, AppConfig& config);

    [[nodiscard]] auto getName() const -> std::string_view override;

    [[nodiscard]] auto getDescription() const -> std::string_view override;

    [[nodiscard]] auto getUsage() const -> std::string_view override;

    [[nodiscard]] auto validateArgs(const std::vector<std::string>& args) const
        -> std::expected<void, std::string> override;

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override;

   private:
    cli::SessionState* m_session;
    AppConfig* m_config;
};

}  // namespace cli::commands
