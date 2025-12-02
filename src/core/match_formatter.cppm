/**
 * @file match_formatter.cppm
 * @brief Match result formatting with index and region classification
 */

module;

#include <bit>
#include <cstdint>
#include <expected>
#include <format>
#include <functional>
#include <optional>
#include <string>
#include <vector>

export module core.match_formatter;

import core.scanner;
import core.region_classifier;
import value.flags;
import ui.show_message;

export namespace core {

/**
 * @struct MatchEntry
 * @brief Single match entry with metadata
 */
struct MatchEntry {
    size_t index;            // 匹配索引
    std::uintptr_t address;  // 内存地址
    std::uint8_t value;      // 当前值（单字节）
    std::string region;      // 区域分类 (heap/stack/code)
};

/**
 * @struct FormatOptions
 * @brief Options for match formatting
 */
struct FormatOptions {
    size_t limit = 20;       // 显示的最大匹配数
    bool showRegion = true;  // 是否显示区域信息
    bool showIndex = true;   // 是否显示索引
};

/**
 * @class MatchFormatter
 * @brief Formats match results with index and region classification
 */
class MatchFormatter {
   public:
    /**
     * @brief Create formatter with optional region classifier
     * @param classifier Optional region classifier for address categorization
     */
    explicit MatchFormatter(
        std::optional<RegionClassifier> classifier = std::nullopt)
        : m_classifier(std::move(classifier)) {}

    /**
     * @brief Format and display matches
     * @param scanner Scanner with match results
     * @param options Formatting options
     * @return Total match count
     */
    [[nodiscard]] auto format(const Scanner& scanner,
                              const FormatOptions& options = {}) const
        -> size_t {
        const auto& matches = scanner.getMatches();

        size_t currentIndex = 0;
        size_t displayCount = 0;
        size_t totalCount = 0;

        std::vector<MatchEntry> entries;
        entries.reserve(options.limit);

        // Collect match entries
        for (const auto& swath : matches.swaths) {
            auto* base = static_cast<std::uint8_t*>(swath.firstByteInChild);
            if (base == nullptr) {
                continue;
            }

            for (size_t i = 0; i < swath.data.size(); ++i) {
                const auto& cell = swath.data[i];
                if (cell.matchInfo == MatchFlags::EMPTY) {
                    continue;
                }

                totalCount++;

                if (displayCount < options.limit) {
                    auto addr = std::bit_cast<std::uintptr_t>(base + i);
                    std::string region = "unk";
                    if (m_classifier && options.showRegion) {
                        region = m_classifier->classify(addr);
                    }

                    entries.push_back(MatchEntry{.index = currentIndex,
                                                 .address = addr,
                                                 .value = cell.oldValue,
                                                 .region = region});

                    displayCount++;
                }

                currentIndex++;
            }

            if (displayCount >= options.limit) {
                // 继续计数但不收集更多条目
                for (size_t i = 0; i < swath.data.size(); ++i) {
                    if (swath.data[i].matchInfo != MatchFlags::EMPTY) {
                        totalCount++;
                    }
                }
            }
        }

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
                    entry.address, static_cast<unsigned>(entry.value),
                    entry.region));
            } else if (options.showIndex) {
                ui::MessagePrinter::info(std::format(
                    "{:<6} 0x{:016x}  0x{:02x}", entry.index, entry.address,
                    static_cast<unsigned>(entry.value)));
            } else if (options.showRegion) {
                ui::MessagePrinter::info(std::format(
                    "0x{:016x}  0x{:02x}      {}", entry.address,
                    static_cast<unsigned>(entry.value), entry.region));
            } else {
                ui::MessagePrinter::info(
                    std::format("0x{:016x}  0x{:02x}", entry.address,
                                static_cast<unsigned>(entry.value)));
            }
        }

        // Print summary
        if (totalCount > options.limit) {
            ui::MessagePrinter::info(
                std::format("\n... and {} more matches (total: {})",
                            totalCount - options.limit, totalCount));
        }

        ui::MessagePrinter::info(std::format("\nShowing {} of {} matches",
                                             displayCount, totalCount));

        return totalCount;
    }

   private:
    std::optional<RegionClassifier> m_classifier;
};

}  // namespace core
