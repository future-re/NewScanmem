/**
 * @file region_classifier.cppm
 * @brief Memory region classification for address categorization
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

export module core.region_classifier;

import core.maps;

export namespace core {

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
    static auto create(pid_t pid)
        -> std::expected<RegionClassifier, std::string> {
        auto mapsRes = readProcessMaps(pid, RegionScanLevel::ALL);
        if (!mapsRes) {
            return std::unexpected(mapsRes.error().message);
        }

        std::vector<RegionLookupEntry> regions;
        regions.reserve(mapsRes->size());

        for (const auto& region : *mapsRes) {
            RegionLookupEntry entry{
                .start = std::bit_cast<std::uintptr_t>(region.start),
                .end =
                    std::bit_cast<std::uintptr_t>(region.start) + region.size,
                .type = region.type,
                .filename = region.filename};
            regions.push_back(entry);
        }

        // Sort by start address for binary search
        std::sort(
            regions.begin(), regions.end(),
            [](const RegionLookupEntry& lhs, const RegionLookupEntry& rhs) {
                return lhs.start < rhs.start;
            });

        return RegionClassifier{std::move(regions)};
    }

    /**
     * @brief Classify an address into a human-readable region description
     * @param addr Address to classify
     * @return Region description string (e.g., "heap", "stack",
     * "code:libfoo.so")
     */
    [[nodiscard]] auto classify(std::uintptr_t addr) const -> std::string {
        for (const auto& entry : m_regions) {
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

    /**
     * @brief Get region type enum for an address
     * @param addr Address to classify
     * @return Region type or nullopt if not found
     */
    [[nodiscard]] auto getRegionType(std::uintptr_t addr) const
        -> std::optional<RegionType> {
        for (const auto& entry : m_regions) {
            if (addr >= entry.start && addr < entry.end) {
                return entry.type;
            }
        }
        return std::nullopt;
    }

   private:
    explicit RegionClassifier(std::vector<RegionLookupEntry> regions)
        : m_regions(std::move(regions)) {}

    static auto regionTypeToString(RegionType regionType) -> std::string_view {
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

    std::vector<RegionLookupEntry> m_regions;
};

}  // namespace core
