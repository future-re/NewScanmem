#pragma once

/**
 * @file write.hpp
 * @brief Write command: modify memory at matched addresses
 */

#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "memseek/cli/command.hpp"
#include "memseek/cli/session.hpp"
#include "memseek/scan/types.hpp"
#include "memseek/value/core.hpp"

namespace cli::commands {

class WriteCommand : public Command {
   public:
    explicit WriteCommand(SessionState& session);

    [[nodiscard]] auto getName() const -> std::string_view override;
    [[nodiscard]] auto getDescription() const -> std::string_view override;
    [[nodiscard]] auto getUsage() const -> std::string_view override;

    [[nodiscard]] auto validateArgs(const std::vector<std::string>& args) const
        -> std::expected<void, std::string> override;

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override;

   private:
    [[nodiscard]] static auto parseWriteUserValue(ScanDataType dataType,
                                                  std::string_view valueText)
        -> std::optional<UserValue>;

    SessionState* m_session;
};

}  // namespace cli::commands
