/**
 * @file region_filter.cppm
 * @brief Region-based filtering for memory scans and match results
 *
 * Provides two filtering modes:
 * 1. Scan-time filtering: Filter regions during initial scan (reduces memory
 * usage)
 * 2. Export-time filtering: Filter match results by region while preserving
 * global indices
 */

module;

#include <algorithm>
#include <bit>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

export module core.region_filter;

import core.maps;              // Region, RegionType
import core.region_classifier; // RegionClassifier

export namespace core {

/**
 * @enum RegionFilterMode
 * @brief Filtering mode selection
 */
enum class RegionFilterMode : uint8_t {
    DISABLED,    // No region filtering
    SCAN_TIME,   // Filter during scan (only scan specified regions)
    EXPORT_TIME  // Filter during export/display (all matches exist, filter on
                 // display)
};

/**
 * @class RegionFilter
 * @brief Manages region-based filtering for memory scanning
 *
 * Supports filtering by region types (exe, code, heap, stack) in two modes:
 * - Scan-time: Filter regions before scanning (memory efficient)
 * - Export-time: Filter matches during result collection (flexible)
 */
class RegionFilter {
   public:
    /**
     * @brief Construct region filter with specified types
     * @param allowedTypes Set of region types to include (empty = all types
     * allowed)
     */
    RegionFilter(std::unordered_set<RegionType> allowedTypes = {})
        : m_allowedTypes(std::move(allowedTypes)) {}

    /**
     * @brief Create filter from region type names
     * @param typeNames Vector of type names ("exe", "code", "heap", "stack",
     * "unknow")
     * @return RegionFilter configured with specified types
     */
    static auto fromTypeNames(const std::vector<std::string>& typeNames)
        -> RegionFilter {
        std::unordered_set<RegionType> types;

        for (const auto& name : typeNames) {
            auto type = stringToRegionType(name);
            if (type) {
                types.insert(*type);
            }
        }

        return RegionFilter{types};
    }

    /**
     * @brief Check if a region type is allowed by this filter
     * @param type Region type to check
     * @return True if type is allowed (or filter is disabled)
     */
    [[nodiscard]] auto isTypeAllowed(RegionType type) const -> bool {
        if (m_allowedTypes.empty()) {
            return true;  // Empty set = allow all
        }
        return m_allowedTypes.contains(type);
    }

    /**
     * @brief Check if a region is allowed by this filter
     * @param region Region to check
     * @return True if region's type is allowed
     */
    [[nodiscard]] auto isRegionAllowed(const Region& region) const -> bool {
        return isTypeAllowed(region.type);
    }

    /**
     * @brief Check if an address is allowed by this filter
     * @param addr Address to check
     * @param classifier Region classifier for address lookup
     * @return True if address is in an allowed region type
     */
    [[nodiscard]] auto isAddressAllowed(
        std::uintptr_t addr, const RegionClassifier& classifier) const -> bool {
        if (m_allowedTypes.empty()) {
            return true;  // No filtering
        }

        // Classify address and extract region type from classification string
        auto classification = classifier.classify(addr);
        auto type = extractRegionTypeFromClassification(classification);
        return type && isTypeAllowed(*type);
    }

    /**
     * @brief Filter a vector of regions to only allowed types
     * @param regions Input regions
     * @return Filtered vector containing only allowed regions
     */
    [[nodiscard]] auto filterRegions(const std::vector<Region>& regions) const
        -> std::vector<Region> {
        if (m_allowedTypes.empty()) {
            return regions;  // No filtering
        }

        std::vector<Region> filtered;
        filtered.reserve(regions.size());

        for (const auto& region : regions) {
            if (isRegionAllowed(region)) {
                filtered.push_back(region);
            }
        }

        return filtered;
    }

    /**
     * @brief Get allowed region types
     * @return Set of allowed region types (empty = all allowed)
     */
    [[nodiscard]] auto getAllowedTypes() const
        -> const std::unordered_set<RegionType>& {
        return m_allowedTypes;
    }

    /**
     * @brief Check if filter is active (has type restrictions)
     * @return True if specific types are filtered, false if all types allowed
     */
    [[nodiscard]] auto isActive() const -> bool {
        return !m_allowedTypes.empty();
    }

    /**
     * @brief Clear filter (allow all types)
     */
    auto clear() -> void { m_allowedTypes.clear(); }

    /**
     * @brief Add allowed region type
     * @param type Region type to allow
     */
    auto addType(RegionType type) -> void { m_allowedTypes.insert(type); }

    /**
     * @brief Remove allowed region type
     * @param type Region type to disallow
     */
    auto removeType(RegionType type) -> void { m_allowedTypes.erase(type); }

    /**
     * @brief Get human-readable description of filter
     * @return String describing allowed types
     */
    [[nodiscard]] auto toString() const -> std::string {
        if (m_allowedTypes.empty()) {
            return "all regions";
        }

        std::string result = "regions: ";
        bool first = true;
        for (const auto& type : m_allowedTypes) {
            if (!first) result += ", ";
            result += std::string(regionTypeToStringLocal(type));
            first = false;
        }
        return result;
    }

   private:
    std::unordered_set<RegionType> m_allowedTypes;

    /**
     * @brief Convert RegionType to string
     * @param type Region type
     * @return String representation
     */
    static auto regionTypeToStringLocal(RegionType type) -> std::string_view {
        switch (type) {
            case RegionType::EXE:
                return "exe";
            case RegionType::CODE:
                return "code";
            case RegionType::HEAP:
                return "heap";
            case RegionType::STACK:
                return "stack";
            case RegionType::UNKNOW:
                return "unknow";
            default:
                return "unknow";
        }
    } /**
       * @brief Convert string to RegionType
       * @param str Type name ("exe", "code", "heap", "stack", "unknow")
       * @return Optional RegionType
       */
    static auto stringToRegionType(const std::string& str)
        -> std::optional<RegionType> {
        if (str == "exe") return RegionType::EXE;
        if (str == "code") return RegionType::CODE;
        if (str == "heap") return RegionType::HEAP;
        if (str == "stack") return RegionType::STACK;
        if (str == "unknow") return RegionType::UNKNOW;
        return std::nullopt;
    }

    /**
     * @brief Extract RegionType from classification string
     * @param classification Classification string from RegionClassifier (e.g.,
     * "heap", "code:lib.so")
     * @return Optional RegionType
     */
    static auto extractRegionTypeFromClassification(
        const std::string& classification) -> std::optional<RegionType> {
        // Classification format: "type" or "type:filename"
        auto colonPos = classification.find(':');
        auto typeStr = (colonPos != std::string::npos)
                           ? classification.substr(0, colonPos)
                           : classification;

        if (typeStr == "unk") return RegionType::UNKNOW;
        return stringToRegionType(typeStr);
    }
};

/**
 * @struct RegionFilterConfig
 * @brief Configuration for region filtering in scans and exports
 */
struct RegionFilterConfig {
    RegionFilterMode mode = RegionFilterMode::DISABLED;
    RegionFilter filter;

    /**
     * @brief Check if filtering is enabled
     * @return True if mode is not DISABLED
     */
    [[nodiscard]] auto isEnabled() const -> bool {
        return mode != RegionFilterMode::DISABLED;
    }

    /**
     * @brief Check if scan-time filtering is enabled
     * @return True if mode is SCAN_TIME
     */
    [[nodiscard]] auto isScanTimeFilter() const -> bool {
        return mode == RegionFilterMode::SCAN_TIME;
    }

    /**
     * @brief Check if export-time filtering is enabled
     * @return True if mode is EXPORT_TIME
     */
    [[nodiscard]] auto isExportTimeFilter() const -> bool {
        return mode == RegionFilterMode::EXPORT_TIME;
    }
};

}  // namespace core
