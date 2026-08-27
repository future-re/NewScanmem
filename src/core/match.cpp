#include "newscanmem/core/match.hpp"

namespace core {
MatchCollector::MatchCollector(std::optional<RegionClassifier> classifier) : m_classifier(std::move(classifier)) {}
auto MatchCollector::collect(const MatchSource& source, const MatchCollectionOptions& options) const -> std::pair<std::vector<MatchEntry>, size_t> {
  if (source.matches == nullptr) return {{}, 0};
  const auto value_size = source.dataType ? bytesNeededForType(*source.dataType) : 1U;
  const auto export_filter = options.regionFilter.isExportTimeFilter() && options.regionFilter.filter.isActive();
  std::vector<MatchEntry> entries; entries.reserve(options.limit); size_t global_index = 0, displayed = 0, total = 0, filtered = 0;
  for (const auto& swath : source.matches->swaths) { auto* base = static_cast<std::uint8_t*>(swath.firstByteInChild); if (base == nullptr) continue;
    for (size_t index = 0; index < swath.data.size(); ++index) { const auto& cell = swath.data[index]; if (cell.matchInfo == MatchFlags::EMPTY) continue; ++total; const auto address = std::bit_cast<std::uintptr_t>(base + index);
      const auto passes = !export_filter || !m_classifier || options.regionFilter.filter.isAddressAllowed(address, *m_classifier);
      if (passes) { ++filtered; if (displayed < options.limit) { entries.push_back({.index = global_index, .address = address, .value = extractValueBytes(swath, index, getActualValueSize(cell, value_size, source.dataType)), .region = getClassifiedRegion(address, options.collectRegion)}); ++displayed; } }
      ++global_index;
    }
  }
  return {std::move(entries), export_filter ? filtered : total};
}
auto MatchCollector::getActualValueSize(const scan::OldValueAndMatchInfo& cell, const size_t fallback, const std::optional<ScanDataType> type) -> size_t { return type && (*type == ScanDataType::STRING || *type == ScanDataType::BYTE_ARRAY) && cell.matchLength > 0 ? cell.matchLength : fallback; }
auto MatchCollector::extractValueBytes(const scan::MatchesAndOldValuesSwath& swath, const size_t start, const size_t count) -> std::vector<std::uint8_t> { std::vector<std::uint8_t> bytes(count); for (size_t index = 0; index < count && start + index < swath.data.size(); ++index) bytes[index] = swath.data[start + index].oldByte; return bytes; }
auto MatchCollector::getClassifiedRegion(const std::uintptr_t address, const bool collect) const -> std::string { return m_classifier && collect ? m_classifier->classify(address) : "unk"; }
}  // namespace core
