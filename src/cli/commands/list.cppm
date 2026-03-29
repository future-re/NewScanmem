/**
 * @file list.cppm
 * @brief List command: display current matches with addresses and values
 */

module;

#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <vector>

export module cli.commands.list;

import app.result_service;
import core.match_formatter;
import cli.command;
import cli.session;
import ui.show_message;
import utils.endianness;

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

        // 解析 limit 参数
        size_t limit = 20;
        if (!args.empty()) {
            try {
                limit = std::stoull(args[0]);
            } catch (...) {
                return std::unexpected("Invalid limit: " + args[0]);
            }
        }

        // Get match entries via service
        auto result = app::ResultService::getMatches(
            {.scanner = m_session->scanner.get(),
             .pid = m_session->pid,
             .limit = limit,
             .showRegion = true,
             .showIndex = true,
             .endianness = m_session->endianness});

        if (!result) {
            return std::unexpected(result.error());
        }

        const auto& [entries, totalCount] = *result;

        ui::MessagePrinter::plain(
            "Index  Address             Size      Region");
        ui::MessagePrinter::plain(
            "-----------------------------------------------");

        for (const auto& entry : entries) {
            ui::MessagePrinter::plain(std::format(
                "{:<6} 0x{:016x}  0x{:02x}      [{}]", entry.index,
                entry.address, entry.value.size(), entry.region));
        }
        ui::MessagePrinter::plain("");
        ui::MessagePrinter::plain(
            std::format("Showing {} of {} matches", entries.size(), totalCount));

        return CommandResult{.success = true, .message = ""};
    }

   private:
    SessionState* m_session;
};

}  // namespace cli::commands
