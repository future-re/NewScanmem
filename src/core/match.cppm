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
import value.flags;
import scan.types; // for ScanDataType, bytesNeededForType

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
    size_t limit = 20;          // 收集的最大匹配数
    bool collectRegion = true;  // 是否收集区域信息
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
     */
    [[nodiscard]] auto collect(const Scanner& scanner,
                               const MatchCollectionOptions& options = {}) const
        -> std::pair<std::vector<MatchEntry>, size_t> {
        const auto& matches = scanner.getMatches();

        // Get data type to determine value size
        auto dataType = scanner.getLastDataType();
        size_t valueSize = dataType ? bytesNeededForType(*dataType) : 1;

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
                    if (m_classifier && options.collectRegion) {
                        region = m_classifier->classify(addr);
                    }

                    // Read complete value bytes from memory
                    std::vector<std::uint8_t> valueBytes(valueSize);
                    for (size_t j = 0;
                         j < valueSize && (i + j) < swath.data.size(); ++j) {
                        valueBytes[j] = swath.data[i + j].oldValue;
                    }

                    entries.push_back(MatchEntry{.index = currentIndex,
                                                 .address = addr,
                                                 .value = std::move(valueBytes),
                                                 .region = region});

                    displayCount++;
                }

                currentIndex++;
            }

            if (displayCount >= options.limit) {
                // 继续计数但不收集更多条目
                for (const auto& info : swath.data) {
                    if (info.matchInfo != MatchFlags::EMPTY) {
                        totalCount++;
                    }
                }
            }
        }

        return {entries, totalCount};
    }

   private:
    std::optional<RegionClassifier> m_classifier;
};

}  // namespace core
