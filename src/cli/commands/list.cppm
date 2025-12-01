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
import value.flags;
import core.maps; // 区域读取与分类

namespace {
// 简单映射 RegionType -> 文本
inline auto regionTypeToString(RegionType regionType) -> std::string_view {
    switch (regionType) {
        case RegionType::HEAP:
            return "heap";
        case RegionType::STACK:
            return "stack";
        case RegionType::EXE:
            return "exe";
        case RegionType::CODE:
            return "code";
        default:
            return "unk";
    }
}

struct RegionLookupEntry {
    std::uintptr_t start;
    std::uintptr_t end;  // half-open
    RegionType type;
    std::string filename;
};

inline auto buildRegionLookup(pid_t pid) -> std::vector<RegionLookupEntry> {
    std::vector<RegionLookupEntry> out;
    auto mapsRes = readProcessMaps(pid, RegionScanLevel::ALL);
    if (!mapsRes) {
        return out;
    }
    out.reserve(mapsRes->size());
    for (const auto& region : *mapsRes) {
        RegionLookupEntry entry{
            std::bit_cast<std::uintptr_t>(region.start),
            std::bit_cast<std::uintptr_t>(region.start) + region.size,
            region.type, region.filename};
        out.push_back(entry);
    }
    std::sort(out.begin(), out.end(),
              [](const RegionLookupEntry& lhs, const RegionLookupEntry& rhs) {
                  return lhs.start < rhs.start;
              });
    return out;
}

inline auto classifyAddress(std::uintptr_t addr,
                            const std::vector<RegionLookupEntry>& regions)
    -> std::string {
    for (const auto& entry : regions) {
        if (addr >= entry.start && addr < entry.end) {
            auto typeStr = regionTypeToString(entry.type);
            if ((entry.type == RegionType::EXE ||
                 entry.type == RegionType::CODE) &&
                !entry.filename.empty()) {
                std::string tail = entry.filename;
                if (tail.size() > 24) {
                    tail = "..." + tail.substr(tail.size() - 21);
                }
                return std::format("{}:{}", typeStr, tail);
            }
            return std::string(typeStr);
        }
    }
    return "unk";
}
}  // namespace

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

        size_t count = 0;         // 已打印数量
        size_t totalMatches = 0;  // 总匹配数量
        size_t globalIndex = 0;   // 全局匹配下标

        ui::MessagePrinter::info(
            "Idx  Address             Region                Value (hex)");
        ui::MessagePrinter::info(
            "--------------------------------------------------------------");

        // 构建区域查找结构
        auto regionLookup = buildRegionLookup(m_session->pid);

        for (const auto& swath : matches.swaths) {
            auto* base = static_cast<std::uint8_t*>(swath.firstByteInChild);
            for (size_t i = 0; i < swath.data.size(); ++i) {
                const auto& cell = swath.data[i];
                if (cell.matchInfo == MatchFlags::EMPTY) {
                    continue;
                }

                totalMatches++;
                auto addr = std::bit_cast<std::uintptr_t>(base + i);
                auto regionStr = classifyAddress(addr, regionLookup);
                if (count < limit) {
                    ui::MessagePrinter::info(std::format(
                        "{:4d} 0x{:016x} {:<20} 0x{:02x}",
                        static_cast<int>(globalIndex), addr, regionStr,
                        static_cast<unsigned>(cell.oldValue)));
                    count++;
                }
                globalIndex++;
            }
        }

        if (totalMatches > limit) {
            ui::MessagePrinter::info(
                std::format("\n... and {} more matches (total: {})",
                            totalMatches - limit, totalMatches));
        }

        ui::MessagePrinter::info(
            std::format("\nShowing {} of {} matches", count, totalMatches));

        return CommandResult{.success = true, .message = ""};
    }

   private:
    SessionState* m_session;
};

}  // namespace cli::commands
