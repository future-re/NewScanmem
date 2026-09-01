#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "memseek/core/maps.hpp"
#include "memseek/core/region_classifier.hpp"

namespace core {
enum class RegionFilterMode : std::uint8_t { DISABLED, SCAN_TIME, EXPORT_TIME };

class RegionFilter {
   public:
    RegionFilter(std::unordered_set<RegionType> allowedTypes = {});
    [[nodiscard]] static auto fromTypeNames(
        const std::vector<std::string>& typeNames) -> RegionFilter;
    [[nodiscard]] auto isTypeAllowed(RegionType type) const -> bool;
    [[nodiscard]] auto isRegionAllowed(const Region& region) const -> bool;
    [[nodiscard]] auto isAddressAllowed(
        std::uintptr_t address,
        const RegionClassifier& classifier) const -> bool;
    [[nodiscard]] auto filterRegions(const std::vector<Region>& regions) const
        -> std::vector<Region>;
    [[nodiscard]] auto getAllowedTypes() const
        -> const std::unordered_set<RegionType>&;
    [[nodiscard]] auto isActive() const -> bool;
    void clear();
    void addType(RegionType type);
    void removeType(RegionType type);
    [[nodiscard]] auto toString() const -> std::string;

   private:
    static auto regionTypeToStringLocal(RegionType type) -> std::string_view;
    static auto stringToRegionType(const std::string& text)
        -> std::optional<RegionType>;
    static auto extractRegionTypeFromClassification(
        const std::string& classification) -> std::optional<RegionType>;
    std::unordered_set<RegionType> m_allowedTypes;
};

struct RegionFilterConfig {
    RegionFilterMode mode{RegionFilterMode::DISABLED};
    RegionFilter filter;
    [[nodiscard]] auto isEnabled() const -> bool {
        return mode != RegionFilterMode::DISABLED;
    }
    [[nodiscard]] auto isScanTimeFilter() const -> bool {
        return mode == RegionFilterMode::SCAN_TIME;
    }
    [[nodiscard]] auto isExportTimeFilter() const -> bool {
        return mode == RegionFilterMode::EXPORT_TIME;
    }
};
}  // namespace core
