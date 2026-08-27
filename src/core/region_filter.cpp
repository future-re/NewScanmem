#include "newscanmem/core/region_filter.hpp"

namespace core {
RegionFilter::RegionFilter(std::unordered_set<RegionType> allowed_types)
    : m_allowedTypes(std::move(allowed_types)) {}
auto RegionFilter::fromTypeNames(const std::vector<std::string>& names)
    -> RegionFilter {
    std::unordered_set<RegionType> types;
    for (const auto& name : names)
        if (const auto type = stringToRegionType(name)) types.insert(*type);
    return RegionFilter{std::move(types)};
}
auto RegionFilter::isTypeAllowed(const RegionType type) const -> bool {
    return m_allowedTypes.empty() || m_allowedTypes.contains(type);
}
auto RegionFilter::isRegionAllowed(const Region& region) const -> bool {
    return isTypeAllowed(region.type);
}
auto RegionFilter::isAddressAllowed(const std::uintptr_t address,
                                    const RegionClassifier& classifier) const
    -> bool {
    if (m_allowedTypes.empty()) return true;
    const auto type =
        extractRegionTypeFromClassification(classifier.classify(address));
    return type && isTypeAllowed(*type);
}
auto RegionFilter::filterRegions(const std::vector<Region>& regions) const
    -> std::vector<Region> {
    if (m_allowedTypes.empty()) return regions;
    std::vector<Region> filtered;
    filtered.reserve(regions.size());
    for (const auto& region : regions)
        if (isRegionAllowed(region)) filtered.push_back(region);
    return filtered;
}
auto RegionFilter::getAllowedTypes() const
    -> const std::unordered_set<RegionType>& {
    return m_allowedTypes;
}
auto RegionFilter::isActive() const -> bool { return !m_allowedTypes.empty(); }
void RegionFilter::clear() { m_allowedTypes.clear(); }
void RegionFilter::addType(const RegionType type) {
    m_allowedTypes.insert(type);
}
void RegionFilter::removeType(const RegionType type) {
    m_allowedTypes.erase(type);
}
auto RegionFilter::toString() const -> std::string {
    if (m_allowedTypes.empty()) return "all regions";
    std::string result{"regions: "};
    for (auto it = m_allowedTypes.begin(); it != m_allowedTypes.end(); ++it) {
        if (it != m_allowedTypes.begin()) result += ", ";
        result += regionTypeToStringLocal(*it);
    }
    return result;
}
auto RegionFilter::regionTypeToStringLocal(const RegionType type)
    -> std::string_view {
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
    }
    return "unknow";
}
auto RegionFilter::stringToRegionType(const std::string& text)
    -> std::optional<RegionType> {
    if (text == "exe") return RegionType::EXE;
    if (text == "code") return RegionType::CODE;
    if (text == "heap") return RegionType::HEAP;
    if (text == "stack") return RegionType::STACK;
    if (text == "unknow") return RegionType::UNKNOW;
    return std::nullopt;
}
auto RegionFilter::extractRegionTypeFromClassification(
    const std::string& classification) -> std::optional<RegionType> {
    const auto colon = classification.find(':');
    const auto type = classification.substr(0, colon);
    return type == "unk" ? std::optional<RegionType>{RegionType::UNKNOW}
                         : stringToRegionType(type);
}
}  // namespace core
