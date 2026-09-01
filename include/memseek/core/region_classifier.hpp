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

#include "memseek/core/maps.hpp"

namespace core {

struct RegionLookupEntry {
    std::uintptr_t start;
    std::uintptr_t end;
    RegionType type;
    std::string filename;
};

class RegionClassifier {
   public:
    static auto create(pid_t pid)
        -> std::expected<RegionClassifier, std::string>;

    [[nodiscard]] auto classify(std::uintptr_t addr) const -> std::string;

    [[nodiscard]] auto getRegionType(std::uintptr_t addr) const
        -> std::optional<RegionType>;

   private:
    explicit RegionClassifier(std::vector<RegionLookupEntry> regions);

    static auto regionTypeToString(RegionType regionType) -> std::string_view;

    std::vector<RegionLookupEntry> m_regions;
};

}  // namespace core
