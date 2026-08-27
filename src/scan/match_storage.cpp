#include "newscanmem/scan/match_storage.hpp"

#include <algorithm>
#include <bit>
#include <cassert>
#include <cctype>
#include <iomanip>
#include <ranges>
#include <sstream>

namespace scan {
void MatchesAndOldValuesSwath::addElement(void* address,
                                          const std::uint8_t byte,
                                          const MatchFlags flags) {
    if (data.empty()) firstByteInChild = address;
    data.push_back({byte, flags, 0});
}
void MatchesAndOldValuesSwath::appendRange(void* base,
                                           const std::uint8_t* bytes,
                                           const std::size_t length,
                                           const MatchFlags initial) {
    if (length == 0) return;
    assert(bytes != nullptr);
    if (data.empty()) firstByteInChild = base;
    for (std::size_t index = 0; index < length; ++index)
        data.push_back({bytes[index], initial, 0});
}
void MatchesAndOldValuesSwath::markRangeByIndex(const std::size_t start,
                                                const std::size_t length,
                                                const MatchFlags flags) {
    if (length == 0 || start >= data.size()) return;
    data[start].matchInfo |= flags;
    data[start].matchLength = length;
}
void MatchesAndOldValuesSwath::markRangeByAddress(void* address,
                                                  const std::size_t length,
                                                  const MatchFlags flags) {
    if (firstByteInChild == nullptr || length == 0) return;
    const auto base = std::bit_cast<std::uintptr_t>(firstByteInChild);
    const auto current = std::bit_cast<std::uintptr_t>(address);
    if (current < base) return;
    markRangeByIndex(static_cast<std::size_t>(current - base), length, flags);
}
auto MatchesAndOldValuesSwath::toPrintableString(
    const std::size_t start, const std::size_t length) const -> std::string {
    if (start >= data.size()) return {};
    std::string result;
    const auto count = std::min(length, data.size() - start);
    result.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        const auto byte = data[start + index].oldByte;
        result += std::isprint(byte) != 0 ? static_cast<char>(byte) : '.';
    }
    return result;
}
auto MatchesAndOldValuesSwath::toByteArrayText(
    const std::size_t start, const std::size_t length) const -> std::string {
    if (start >= data.size()) return {};
    std::ostringstream output;
    const auto count = std::min(length, data.size() - start);
    output << std::nouppercase << std::hex;
    for (std::size_t index = 0; index < count; ++index) {
        output << std::setw(2) << std::setfill('0')
               << static_cast<unsigned>(data[start + index].oldByte);
        if (index + 1 < count) output << ' ';
    }
    return output.str();
}
void MatchesAndOldValuesArray::addSwath(const MatchesAndOldValuesSwath& swath) {
    swaths.push_back(swath);
}
void MatchesAndOldValuesArray::addSwath(MatchesAndOldValuesSwath&& swath) {
    swaths.push_back(std::move(swath));
}
auto MatchesAndOldValuesArray::nthMatch(const std::size_t target)
    -> std::optional<std::pair<MatchesAndOldValuesSwath*, std::size_t>> {
    std::size_t count = 0;
    for (auto& swath : swaths)
        for (std::size_t index = 0; index < swath.data.size(); ++index)
            if (swath.data[index].matchInfo != MatchFlags::EMPTY) {
                if (count++ == target) return std::make_pair(&swath, index);
            }
    return std::nullopt;
}
void MatchesAndOldValuesArray::deleteInAddressRange(void* start, void* end,
                                                    unsigned long& matches) {
    matches = 0;
    const auto lower = std::bit_cast<std::uintptr_t>(start);
    const auto upper = std::bit_cast<std::uintptr_t>(end);
    if (start == nullptr || end == nullptr || lower >= upper) return;
    for (auto& swath : swaths) {
        if (swath.firstByteInChild == nullptr || swath.data.empty()) continue;
        const auto base = std::bit_cast<std::uintptr_t>(swath.firstByteInChild);
        const auto swath_end = base + swath.data.size();
        const auto clamped_start = std::max(lower, base);
        const auto clamped_end = std::min(upper, swath_end);
        if (clamped_start >= clamped_end) continue;
        const auto first = static_cast<std::size_t>(clamped_start - base);
        const auto last = static_cast<std::size_t>(clamped_end - base);
        for (std::size_t index = first; index < last; ++index)
            if (swath.data[index].matchInfo != MatchFlags::EMPTY) ++matches;
        swath.data.erase(
            swath.data.begin() + static_cast<std::ptrdiff_t>(first),
            swath.data.begin() + static_cast<std::ptrdiff_t>(last));
        if (first == 0)
            swath.firstByteInChild = std::bit_cast<void*>(clamped_end);
    }
    const auto remove = std::ranges::remove_if(
        swaths, [](const auto& swath) { return swath.data.empty(); });
    swaths.erase(remove.begin(), remove.end());
}
auto MatchesAndOldValuesArray::getRawBytesAt(
    void* address, const std::size_t length,
    std::vector<std::uint8_t>& output) const -> bool {
    if (address == nullptr || length == 0) return false;
    const auto target = std::bit_cast<std::uintptr_t>(address);
    for (const auto& swath : swaths) {
        if (swath.firstByteInChild == nullptr || swath.data.empty()) continue;
        const auto base = std::bit_cast<std::uintptr_t>(swath.firstByteInChild);
        if (target < base) continue;
        const auto offset = static_cast<std::size_t>(target - base);
        if (offset > swath.data.size() || length > swath.data.size() - offset)
            continue;
        output.resize(length);
        for (std::size_t index = 0; index < length; ++index)
            output[index] = swath.data[offset + index].oldByte;
        return true;
    }
    return false;
}
}  // namespace scan
