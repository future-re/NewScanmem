module;

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

export module targetmem;

import value;
import show_message;

// Interface: exported types and functions
export {
    struct OldValueAndMatchInfo {
        uint8_t old_value;
        MatchFlags match_info;
    };

    class MatchesAndOldValuesSwath {
       public:
        void* firstByteInChild = nullptr;
        std::vector<OldValueAndMatchInfo> data;

        MatchesAndOldValuesSwath() = default;

        void addElement(void* addr, uint8_t byte, MatchFlags matchFlags) {
            if (data.empty()) {
                firstByteInChild = addr;
            }
            data.push_back({byte, matchFlags});
        }

        [[nodiscard]] auto toPrintableString(size_t idx, size_t len) const -> std::string {
            std::ostringstream oss;
            size_t count = std::min(len, data.size() - idx);
            for (size_t i = 0; i < count; ++i) {
                auto byteVal = data[idx + i].old_value;
                oss << ((std::isprint(byteVal) != 0) ? static_cast<char>(byteVal)
                                              : '.');
            }
            return oss.str();
        }

        [[nodiscard]] auto toByteArrayText(size_t idx, size_t len) const -> std::string {
            std::ostringstream oss;
            size_t count = std::min(len, data.size() - idx);
            for (size_t i = 0; i < count; ++i) {
                oss << std::hex << static_cast<int>(data[idx + i].old_value);
                if (i + 1 < count) {
                    oss << " ";
                }
            }
            return oss.str();
        }
    };

    class MatchesAndOldValuesArray {
       public:
        size_t maxNeededBytes;
        std::vector<MatchesAndOldValuesSwath> swaths;

        MatchesAndOldValuesArray(size_t maxBytes) : maxNeededBytes(maxBytes) {}

        void addSwath(const MatchesAndOldValuesSwath& swath) {
            swaths.push_back(swath);
        }

        auto nthMatch(
            size_t n) -> std::optional<std::pair<MatchesAndOldValuesSwath*, size_t>> {
            size_t count = 0;
            for (auto& swath : swaths) {
                for (size_t i = 0; i < swath.data.size(); ++i) {
                    if (swath.data[i].match_info != MatchFlags::EMPTY) {
                        if (count == n) {
                            return std::make_pair(&swath, i);
                        }
                        ++count;
                    }
                }
            }
            return std::nullopt;
        }

        void deleteInAddressRange(void* start, void* end,
                                  unsigned long& numMatches) {
            numMatches = 0;
            for (auto& swath : swaths) {
                auto iter = std::ranges::remove_if(
                    swath.data,
                    [&](const OldValueAndMatchInfo& info) {
                        void* addr =
                            static_cast<char*>(swath.firstByteInChild) +
                            (&info - swath.data.data());
                        if (addr >= start && addr < end) {
                            if (info.match_info != MatchFlags::EMPTY) {
                                ++numMatches;
                            }
                            return true;
                        }
                        return false;
                    });
                swath.data.erase(iter.begin(), swath.data.end());
            }
        }
    };
}