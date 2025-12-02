/**
 * @file list.cppm
 * @brief List command: display current matches with addresses and values
 */

module;

#include <algorithm>
#include <bit>
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
import core.region_classifier;
import core.match_formatter;

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

        // 创建区域分类器（可选）
        auto classifierRes = core::RegionClassifier::create(m_session->pid);
        std::optional<core::RegionClassifier> classifier;
        if (classifierRes) {
            classifier = std::move(*classifierRes);
        }

        // 使用格式化器显示结果
        core::MatchFormatter formatter{std::move(classifier)};
        core::FormatOptions options{
            .limit = limit, .showRegion = true, .showIndex = true};

        auto* scanner = m_session->scanner.get();
        (void)formatter.format(*scanner, options);

        return CommandResult{.success = true, .message = ""};
    }

   private:
    SessionState* m_session;
};

}  // namespace cli::commands
