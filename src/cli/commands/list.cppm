/**
 * @file list.cppm
 * @brief List command: display current matches with addresses and values
 */

module;

#include <cstdint>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <vector>

export module cli.commands.list;

import cli.command;
import cli.session;
import ui.show_message;
import core.scanner;
import value.flags;

export namespace cli::commands {

class ListCommand : public Command {
   public:
    explicit ListCommand(SessionState& session) : m_session(&session) {}

    [[nodiscard]] auto getName() const -> std::string_view override {
        return "list";
    }

    [[nodiscard]] auto getDescription() const -> std::string_view override {
        return "List current matches with addresses and values";
    }

    [[nodiscard]] auto getUsage() const -> std::string_view override {
        return "list [limit]\n"
               "  limit (可选): 显示的最大匹配数 (默认: 20)\n"
               "  示例: list / list 50";
    }

    [[nodiscard]] auto execute(const std::vector<std::string>& args)
        -> std::expected<CommandResult, std::string> override {
        if (m_session == nullptr || !m_session->scanner) {
            return std::unexpected("No scanner initialized. Run a scan first.");
        }

        auto* scanner = m_session->scanner.get();
        const auto& matches = scanner->getMatches();

        size_t limit = 20;
        if (!args.empty()) {
            try {
                limit = std::stoull(args[0]);
            } catch (...) {
                return std::unexpected("Invalid limit: " + args[0]);
            }
        }

        size_t count = 0;
        size_t total = 0;

        ui::MessagePrinter::info("Address             Value (hex)");
        ui::MessagePrinter::info("-----------------------------------");

        for (const auto& swath : matches.swaths) {
            auto* base = static_cast<std::uint8_t*>(swath.firstByteInChild);
            for (size_t i = 0; i < swath.data.size(); ++i) {
                const auto& cell = swath.data[i];
                if (cell.matchInfo == MatchFlags::EMPTY) {
                    continue;
                }

                total++;
                if (count >= limit) {
                    continue;
                }

                std::uintptr_t addr = std::bit_cast<std::uintptr_t>(base + i);

                // 显示地址和旧值
                ui::MessagePrinter::info(
                    std::format("0x{:016x}  0x{:02x}", addr,
                                static_cast<unsigned>(cell.oldValue)));

                count++;
            }
        }

        if (total > limit) {
            ui::MessagePrinter::info(std::format(
                "\n... and {} more matches (total: {})", total - limit, total));
        }

        ui::MessagePrinter::info(
            std::format("\nShowing {} of {} matches", count, total));

        return CommandResult{.success = true, .message = ""};
    }

   private:
    SessionState* m_session;
};

}  // namespace cli::commands
