#include "memseek/cli/commands/list.hpp"

#include <format>

#include "memseek/app/result_service.hpp"
#include "memseek/ui/show_message.hpp"

namespace cli::commands {

ListCommand::ListCommand(SessionState& session) : m_session(&session) {}

auto ListCommand::getName() const -> std::string_view { return "list"; }

auto ListCommand::getDescription() const -> std::string_view {
    return "List current matches with addresses and values";
}

auto ListCommand::getUsage() const -> std::string_view {
    return "list [limit]\n"
           "  limit (可选): 显示的最大匹配数 (默认: 20)\n"
           "  示例: list / list 50";
}

auto ListCommand::execute(const std::vector<std::string>& args)
    -> std::expected<CommandResult, std::string> {
    if (m_session == nullptr || !m_session->scanner)
        return std::unexpected("No scanner initialized. Run a scan first.");
    size_t limit = 20;
    if (!args.empty()) {
        try {
            limit = std::stoull(args[0]);
        } catch (...) {
            return std::unexpected("Invalid limit: " + args[0]);
        }
    }
    auto result =
        app::ResultService::getMatches({.scanner = m_session->scanner.get(),
                                        .pid = m_session->pid,
                                        .limit = limit,
                                        .showRegion = true,
                                        .showIndex = true,
                                        .endianness = m_session->endianness});
    if (!result) return std::unexpected(result.error());
    const auto& [entries, totalCount] = *result;
    ui::MessagePrinter::plain("Index  Address             Size      Region");
    ui::MessagePrinter::plain(
        "-----------------------------------------------");
    for (const auto& entry : entries) {
        ui::MessagePrinter::plain(
            std::format("{:<6} 0x{:016x}  0x{:02x}      [{}]", entry.index,
                        entry.address, entry.value.size(), entry.region));
    }
    ui::MessagePrinter::plain("");
    ui::MessagePrinter::plain(
        std::format("Showing {} of {} matches", entries.size(), totalCount));
    return CommandResult{.success = true, .message = ""};
}

}  // namespace cli::commands
