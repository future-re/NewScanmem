#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "newscanmem/value/flags.hpp"

namespace scan {
struct OldValueAndMatchInfo { std::uint8_t oldByte{}; MatchFlags matchInfo{MatchFlags::EMPTY}; std::size_t matchLength{}; };
class MatchesAndOldValuesSwath {
 public:
  void* firstByteInChild{nullptr}; std::vector<OldValueAndMatchInfo> data;
  void addElement(void* address, std::uint8_t byte, MatchFlags flags);
  void appendRange(void* base_address, const std::uint8_t* bytes, std::size_t length, MatchFlags initial = MatchFlags::EMPTY);
  void markRangeByIndex(std::size_t start_index, std::size_t length, MatchFlags flags);
  void markRangeByAddress(void* address, std::size_t length, MatchFlags flags);
  [[nodiscard]] auto toPrintableString(std::size_t index, std::size_t length) const -> std::string;
  [[nodiscard]] auto toByteArrayText(std::size_t index, std::size_t length) const -> std::string;
};
class MatchesAndOldValuesArray {
 public:
  std::vector<MatchesAndOldValuesSwath> swaths;
  void addSwath(const MatchesAndOldValuesSwath& swath);
  void addSwath(MatchesAndOldValuesSwath&& swath);
  [[nodiscard]] auto nthMatch(std::size_t index) -> std::optional<std::pair<MatchesAndOldValuesSwath*, std::size_t>>;
  void deleteInAddressRange(void* start, void* end, unsigned long& match_count);
  [[nodiscard]] auto getRawBytesAt(void* address, std::size_t length, std::vector<std::uint8_t>& output) const -> bool;
};
}  // namespace scan
