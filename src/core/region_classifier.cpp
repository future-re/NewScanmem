#include "newscanmem/core/region_classifier.hpp"

namespace core {
RegionClassifier::RegionClassifier(std::vector<RegionLookupEntry> regions)
    : m_regions(std::move(regions)) {}
auto RegionClassifier::create(const pid_t pid)
    -> std::expected<RegionClassifier, std::string> {
    const auto maps = readProcessMaps(pid, RegionScanLevel::ALL);
    if (!maps) return std::unexpected(maps.error().message);
    std::vector<RegionLookupEntry> regions;
    regions.reserve(maps->size());
    for (const auto& region : *maps)
        regions.push_back(
            {.start = std::bit_cast<std::uintptr_t>(region.start),
             .end = std::bit_cast<std::uintptr_t>(region.start) + region.size,
             .type = region.type,
             .filename = region.filename});
    std::ranges::sort(regions, {}, &RegionLookupEntry::start);
    return RegionClassifier{std::move(regions)};
}
auto RegionClassifier::classify(const std::uintptr_t address) const
    -> std::string {
    const auto type = getRegionType(address);
    if (!type) return "unk";
    const auto entry =
        std::ranges::find_if(m_regions, [address](const auto& item) {
            return address >= item.start && address < item.end;
        });
    if ((*type == RegionType::EXE || *type == RegionType::CODE) &&
        !entry->filename.empty()) {
        auto tail = entry->filename;
        if (tail.size() > 24) tail = "..." + tail.substr(tail.size() - 21);
        return std::format("{}:{}", regionTypeToString(*type), tail);
    }
    return std::string(regionTypeToString(*type));
}
auto RegionClassifier::getRegionType(const std::uintptr_t address) const
    -> std::optional<RegionType> {
    const auto entry =
        std::ranges::find_if(m_regions, [address](const auto& item) {
            return address >= item.start && address < item.end;
        });
    return entry == m_regions.end() ? std::nullopt
                                    : std::optional<RegionType>{entry->type};
}
auto RegionClassifier::regionTypeToString(const RegionType type)
    -> std::string_view {
    switch (type) {
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
}  // namespace core
