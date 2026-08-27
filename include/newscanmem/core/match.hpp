#pragma once

/**
 * @file match.hpp
 * @brief Match data structures and collection logic
 */

#include <bit>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "newscanmem/core/region_classifier.hpp"
#include "newscanmem/core/region_filter.hpp"
#include "newscanmem/value/flags.hpp"
#include "newscanmem/scan/types.hpp"
#include "newscanmem/scan/match_storage.hpp"

namespace core {

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

struct MatchSource {
    const scan::MatchesAndOldValuesArray* matches{nullptr};
    std::optional<ScanDataType> dataType;
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
    explicit MatchCollector(std::optional<RegionClassifier> classifier = std::nullopt);

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
    [[nodiscard]] auto collect(const MatchSource& source, const MatchCollectionOptions& options = {}) const
        -> std::pair<std::vector<MatchEntry>, size_t>;

   private:
    [[nodiscard]] static auto getActualValueSize(const scan::OldValueAndMatchInfo& cell,
        size_t default_value_size, std::optional<ScanDataType> data_type) -> size_t;

    [[nodiscard]] static auto extractValueBytes(const scan::MatchesAndOldValuesSwath& swath,
        size_t start_index, size_t count) -> std::vector<std::uint8_t>;

    [[nodiscard]] auto getClassifiedRegion(std::uintptr_t address, bool collect_region) const -> std::string;
    std::optional<RegionClassifier> m_classifier;
};

}  // namespace core
