#pragma once

/**
 * @file region_classifier.hpp
 * @brief Memory region classification for address categorization
 */

#include <algorithm>
#include <bit>
#include <cstdint>
#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "newscanmem/core/maps.hpp"

namespace core {

/**
 * @struct RegionLookupEntry
 * @brief Cached region information for fast address classification
 */
struct RegionLookupEntry {
    std::uintptr_t start;
    std::uintptr_t end;  // half-open range
    RegionType type;
    std::string filename;
};

/**
 * @class RegionClassifier
 * @brief Classifies memory addresses into region types (heap/stack/code/etc)
 */
class RegionClassifier {
   public:
    /**
     * @brief Initialize classifier for a given process
     * @param pid Target process ID
     * @return Expected with classifier or error message
     */
    static auto create(pid_t pid) -> std::expected<RegionClassifier, std::string>;

    /**
     * @brief Classify an address into a human-readable region description
     * @param addr Address to classify
     * @return Region description string (e.g., "heap", "stack",
     * "code:libfoo.so")
     */
    [[nodiscard]] auto classify(std::uintptr_t addr) const -> std::string;

    /**
     * @brief Get region type enum for an address
     * @param addr Address to classify
     * @return Region type or nullopt if not found
     */
    [[nodiscard]] auto getRegionType(std::uintptr_t addr) const -> std::optional<RegionType>;

   private:
    explicit RegionClassifier(std::vector<RegionLookupEntry> regions);

    static auto regionTypeToString(RegionType region_type) -> std::string_view;

    std::vector<RegionLookupEntry> m_regions;
};

}  // namespace core
