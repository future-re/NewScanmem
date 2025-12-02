/**
 * @file match_formatter.cppm
 * @brief Match result formatting and display
 */

module;

#include <format>
#include <vector>

export module core.match_formatter;

import core.match;
import ui.show_message;

export namespace core {

/**
 * @struct FormatOptions
 * @brief Options for match formatting and display
 */
struct FormatOptions {
    bool showRegion = true;         // 是否显示区域信息
    bool showIndex = true;          // 是否显示索引
    bool bigEndianDisplay = false;  // 是否按大端显示数值（与读取端序一致）
};

/**
 * @class MatchFormatter
 * @brief Formats and displays match entries
 */
class MatchFormatter {
   public:
    /**
     * @brief Format and display match entries
     * @param entries Match entries to display
     * @param totalCount Total number of matches
     * @param options Formatting options
     */
    static auto display(const std::vector<MatchEntry>& entries,
                        size_t totalCount, const FormatOptions& options = {})
        -> void {
        // Print header
        if (options.showIndex && options.showRegion) {
            ui::MessagePrinter::info(
                "Index  Address             Value     Region");
            ui::MessagePrinter::info(
                "-----------------------------------------------");
        } else if (options.showIndex) {
            ui::MessagePrinter::info("Index  Address             Value");
            ui::MessagePrinter::info("---------------------------------------");
        } else if (options.showRegion) {
            ui::MessagePrinter::info("Address             Value     Region");
            ui::MessagePrinter::info("---------------------------------------");
        } else {
            ui::MessagePrinter::info("Address             Value");
            ui::MessagePrinter::info("-----------------------------------");
        }

        // Print entries
        for (const auto& entry : entries) {
            if (options.showIndex && options.showRegion) {
                ui::MessagePrinter::info(std::format(
                    "{:<6} 0x{:016x}  0x{:02x}      {}", entry.index,
                    entry.address,
                    static_cast<unsigned>(options.bigEndianDisplay
                                              ? std::byteswap(entry.value)
                                              : entry.value),
                    entry.region));
            } else if (options.showIndex) {
                ui::MessagePrinter::info(std::format(
                    "{:<6} 0x{:016x}  0x{:02x}", entry.index, entry.address,
                    static_cast<unsigned>(options.bigEndianDisplay
                                              ? std::byteswap(entry.value)
                                              : entry.value)));
            } else if (options.showRegion) {
                ui::MessagePrinter::info(std::format(
                    "0x{:016x}  0x{:02x}      {}", entry.address,
                    static_cast<unsigned>(options.bigEndianDisplay
                                              ? std::byteswap(entry.value)
                                              : entry.value),
                    entry.region));
            } else {
                ui::MessagePrinter::info(std::format(
                    "0x{:016x}  0x{:02x}", entry.address,
                    static_cast<unsigned>(options.bigEndianDisplay
                                              ? std::byteswap(entry.value)
                                              : entry.value)));
            }
        }

        // Print summary
        size_t displayCount = entries.size();
        if (totalCount > displayCount) {
            ui::MessagePrinter::info(
                std::format("\n... and {} more matches (total: {})",
                            totalCount - displayCount, totalCount));
        }

        ui::MessagePrinter::info(std::format("\nShowing {} of {} matches",
                                             displayCount, totalCount));
    }
};

}  // namespace core
