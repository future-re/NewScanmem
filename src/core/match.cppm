/**
 * @file match.cppm
 * @brief Match data structures and collection logic
 */

module;

#include <bit>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

export module core.match;

import core.scanner;
import core.region_classifier;
import core.region_filter;
import value.flags;
import scan.types;         // for ScanDataType, bytesNeededForType
import scan.match_storage; // for MatchesAndOldValuesSwath, OldValueAndMatchInfo

export namespace core {

/**
 * @struct MatchEntry
 * @brief Single match entry with metadata
 */
struct MatchEntry {
    size_t index;                     // 匹配索引
    std::uintptr_t address;           // 内存地址
    std::vector<std::uint8_t> value;  // 当前值（完整字节序列）
    std::string region;               // 区域分类 (heap/stack/code)
};

/**
 * @struct MatchCollectionOptions
 * @brief Options for match collection
 */
struct MatchCollectionOptions {
    size_t limit = 100;         // 收集的最大匹配数
    bool collectRegion = true;  // 是否收集区域信息
    RegionFilterConfig
        regionFilter;  // Region filtering for export-time filtering
};

/**
 * @class MatchCollector
 * @brief Collects match entries from scanner results
 */
class MatchCollector {
   public:
    /**
     * @brief Create collector with optional region classifier
     * @param classifier Optional region classifier for address categorization
     */
    explicit MatchCollector(
        std::optional<RegionClassifier> classifier = std::nullopt)
        : m_classifier(std::move(classifier)) {}

    /**
     * @brief Collect match entries from scanner
     * @param scanner Scanner with match results
     * @param options Collection options
     * @return Pair of (collected entries, total match count)
     *
     * Note: When export-time filtering is enabled, the index field represents
     * the global index (position among ALL matches), not the filtered index.
     * This ensures indices remain stable regardless of filtering.
     */
    // NOLINTNEXTLINE
    [[nodiscard]] auto collect(const Scanner& scanner,
                               const MatchCollectionOptions& options = {}) const
        -> std::pair<std::vector<MatchEntry>, size_t> {
        const auto& matches = scanner.getMatches();
        auto dataType = scanner.getLastDataType();
        size_t valueSize = dataType ? bytesNeededForType(*dataType) : 1;

        size_t globalIndex = 0;
        size_t displayCount = 0;
        size_t totalCount = 0;
        size_t filteredCount = 0;

        std::vector<MatchEntry> entries;
        entries.reserve(options.limit);

        bool useExportFilter = options.regionFilter.isExportTimeFilter() &&
                               options.regionFilter.filter.isActive();

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
                auto addr = std::bit_cast<std::uintptr_t>(base + i);

                bool passesFilter = true;
                if (useExportFilter && m_classifier) {
                    passesFilter = options.regionFilter.filter.isAddressAllowed(
                        addr, *m_classifier);
                }

                if (passesFilter) {
                    filteredCount++;

                    if (displayCount < options.limit) {
                        size_t actualSize =
                            getActualValueSize(cell, valueSize, dataType);
                        auto valueBytes =
                            extractValueBytes(swath, i, actualSize);
                        std::string region =
                            getClassifiedRegion(addr, options.collectRegion);

                        entries.push_back(
                            MatchEntry{.index = globalIndex,
                                       .address = addr,
                                       .value = std::move(valueBytes),
                                       .region = std::move(region)});
                        displayCount++;
                    }
                }
                globalIndex++;
            }
        }

        size_t effectiveTotal = useExportFilter ? filteredCount : totalCount;
        return {entries, effectiveTotal};
    }

   private:
    [[nodiscard]] static auto getActualValueSize(
        const scan::OldValueAndMatchInfo& cell, size_t defaultValueSize,
        std::optional<ScanDataType> dataType) -> size_t {
        if (dataType && (*dataType == ScanDataType::STRING ||
                         *dataType == ScanDataType::BYTE_ARRAY)) {
            if (cell.matchLength > 0) {
                return cell.matchLength;
            }
        }
        return defaultValueSize;
    }

    [[nodiscard]] static auto extractValueBytes(
        const scan::MatchesAndOldValuesSwath& swath, size_t startIndex,
        size_t count) -> std::vector<std::uint8_t> {
        std::vector<std::uint8_t> bytes(count);
        for (size_t j = 0; j < count && (startIndex + j) < swath.data.size();
             ++j) {
            bytes[j] = swath.data[startIndex + j].oldByte;
        }
        return bytes;
    }

    [[nodiscard]] auto getClassifiedRegion(std::uintptr_t addr,
                                           bool collectRegion) const
        -> std::string {
        if (m_classifier && collectRegion) {
            return m_classifier->classify(addr);
        }
        return "unk";
    }
    std::optional<RegionClassifier> m_classifier;
};

}  // namespace core
