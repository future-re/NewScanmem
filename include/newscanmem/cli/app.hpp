#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "newscanmem/cli/app_config.hpp"
#include "newscanmem/cli/session.hpp"
#include "newscanmem/ui/interface.hpp"

namespace cli {
class Application {
   public:
    explicit Application(const AppConfig& config);
    auto run() -> int;

   private:
    auto executeCommandString(const std::string& commands) -> bool;
    static auto getCommandCompletions(std::string_view prefix)
        -> std::vector<std::string>;
    auto registerCommands() -> void;
    [[nodiscard]] auto buildPrompt() const -> std::string;
    AppConfig m_config;
    SessionState m_session;
    std::shared_ptr<ui::UserInterface> m_ui;
};
}  // namespace cli
