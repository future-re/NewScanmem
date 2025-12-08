/**
 * @file match_storage.cppm
 * @brief Types for storing scan results and historical bytes.
 */

module;

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <optional>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

export module scan.match_storage;

import value.flags;

export namespace scan {

// Exported types and functions

/** @brief Historical byte value and its match flags */
struct OldValueAndMatchInfo {
    uint8_t oldByte;       ///< Historical byte value (single byte)
    MatchFlags matchInfo;  ///< Match flags for this byte
};

class MatchesAndOldValuesSwath {
   public:
    void* firstByteInChild = nullptr;
    std::vector<OldValueAndMatchInfo> data;

    MatchesAndOldValuesSwath() = default;

    /* Append a single byte and its match flags; set firstByteInChild on first
     * insert. Caller should append bytes in ascending address order. */
    void addElement(void* addr, uint8_t byte, MatchFlags matchFlags) {
        if (data.empty()) {
            firstByteInChild = addr;
        }
        data.push_back({byte, matchFlags});
    }

    // Append multiple bytes
    void appendRange(void* baseAddr, const uint8_t* bytes, size_t length,
                     MatchFlags initial = MatchFlags::EMPTY) {
        if (length == 0) {
            return;
        }
        assert(bytes != nullptr);
        if (data.empty()) {
            firstByteInChild = baseAddr;
        }
        for (std::size_t i = 0; i < length; ++i) {
            data.push_back({bytes[i], initial});
        }
    }

    // Mark a contiguous index range with flags
    void markRangeByIndex(size_t startIndex, size_t length, MatchFlags flags) {
        if (length == 0 || startIndex >= data.size()) {
            return;
        }
        size_t endIndex = std::min(startIndex + length, data.size());
        for (size_t i = startIndex; i < endIndex; ++i) {
            data[i].matchInfo = data[i].matchInfo | flags;
        }
    }

    // Mark a target address range with flags
    void markRangeByAddress(void* addr, size_t length, MatchFlags flags) {
        if ((firstByteInChild == nullptr) || length == 0) {
            return;
        }
        const auto* base = static_cast<const char*>(firstByteInChild);
        const auto* curr = static_cast<const char*>(addr);
        if (curr < base) {
            return;
        }
        auto offset = static_cast<size_t>(curr - base);
        markRangeByIndex(offset, length, flags);
    }

    /* Return a printable string for up to len bytes starting at idx.
     * Non-printable bytes are shown as '.'. */
    [[nodiscard]] auto toPrintableString(size_t idx, size_t len) const
        -> std::string {
        std::ostringstream oss;
        if (idx >= data.size()) {
            return {};
        }
        size_t count = std::min(len, data.size() - idx);
        for (size_t i = 0; i < count; ++i) {
            auto byteVal = data[idx + i].oldByte;
            oss << ((std::isprint(byteVal) != 0) ? static_cast<char>(byteVal)
                                                 : '.');
        }
        return oss.str();
    }

    /* Return hex text (space-separated) for up to len bytes starting at idx. */
    [[nodiscard]] auto toByteArrayText(size_t idx, size_t len) const
        -> std::string {
        std::ostringstream oss;
        if (idx >= data.size()) {
            return {};
        }
        size_t count = std::min(len, data.size() - idx);
        oss << std::nouppercase << std::hex;
        for (size_t i = 0; i < count; ++i) {
            oss << std::setw(2) << std::setfill('0')
                << (static_cast<int>(data[idx + i].oldByte) &
                    0xFF);  // NOLINT(readability-magic-numbers)
            if (i + 1 < count) {
                oss << ' ';
            }
        }
        return oss.str();
    }
};

/* MatchesAndOldValuesArray: collection of swaths storing historical bytes
 * and match flags. */
class MatchesAndOldValuesArray {
   public:
    std::vector<MatchesAndOldValuesSwath> swaths;
    MatchesAndOldValuesArray() = default;

    /* Append a swath (copied). Caller may assume ordered insertion by address.
     */
    void addSwath(const MatchesAndOldValuesSwath& swath) {
        swaths.push_back(swath);
    }

    /* Return pointer and index for the n-th match, or std::nullopt if not
     * found. */
    auto nthMatch(size_t n)
        -> std::optional<std::pair<MatchesAndOldValuesSwath*, size_t>> {
        size_t count = 0;
        for (auto& swath : swaths) {
            for (size_t i = 0; i < swath.data.size(); ++i) {
                if (swath.data[i].matchInfo != MatchFlags::EMPTY) {
                    if (count == n) {
                        return std::make_pair(&swath, i);
                    }
                    ++count;
                }
            }
        }
        return std::nullopt;
    }

    /* Remove bytes in [start, end); count deleted matches in numMatches. */
    void deleteInAddressRange(void* start, void* end,
                              unsigned long& numMatches) {
        numMatches = 0;
        if (start == nullptr || end == nullptr || start == end) {
            return;
        }

        for (auto& swath : swaths) {
            if (swath.firstByteInChild == nullptr || swath.data.empty()) {
                continue;
            }

            auto* basePtr = static_cast<char*>(swath.firstByteInChild);
            const auto* startPtr = static_cast<const char*>(start);
            const auto* endPtr = static_cast<const char*>(end);

            const std::size_t SWATH_SIZE = swath.data.size();
            const auto* swathEndPtr =
                basePtr + static_cast<std::ptrdiff_t>(SWATH_SIZE);

            // Clamp to swath bounds
            const auto* clampedStart =
                std::max(startPtr, static_cast<const char*>(basePtr));
            const auto* clampedEnd = std::min(endPtr, swathEndPtr);

            if (clampedStart >= clampedEnd) {
                continue;  // no intersection
            }

            const std::size_t START_IDX =
                static_cast<std::size_t>(clampedStart - basePtr);
            const std::size_t END_IDX =
                static_cast<std::size_t>(clampedEnd - basePtr);

            // Count matches in the interval
            for (std::size_t i = START_IDX; i < END_IDX; ++i) {
                if (swath.data[i].matchInfo != MatchFlags::EMPTY) {
                    ++numMatches;
                }
            }

            // Erase bytes in [startIdx, endIdx)
            swath.data.erase(
                swath.data.begin() + static_cast<std::ptrdiff_t>(START_IDX),
                swath.data.begin() + static_cast<std::ptrdiff_t>(END_IDX));

            // If we erased a prefix, advance the base address
            if (START_IDX == 0) {
                swath.firstByteInChild =
                    basePtr + static_cast<std::ptrdiff_t>(END_IDX);
            }
        }

        // Drop empty swaths to keep structure compact
        const auto REMOVE_EMPTY = std::ranges::remove_if(
            swaths, [](const MatchesAndOldValuesSwath& swathEntry) {
                return swathEntry.data.empty();
            });
        swaths.erase(REMOVE_EMPTY.begin(), REMOVE_EMPTY.end());
    }

    /* Attempt to read len historical bytes starting at addr into out; return
     * true on success. */
    [[nodiscard]] auto getRawBytesAt(void* addr, size_t len,
                                     std::vector<uint8_t>& out) const -> bool {
        if (addr == nullptr || len == 0) {
            return false;
        }
        for (const auto& swath : swaths) {
            if (swath.firstByteInChild == nullptr || swath.data.empty()) {
                continue;
            }
            const auto* base = static_cast<const char*>(swath.firstByteInChild);
            const auto* curr = static_cast<const char*>(addr);
            if (curr < base) {
                continue;
            }
            const auto OFFSET = static_cast<size_t>(curr - base);
            if (OFFSET >= swath.data.size()) {
                continue;
            }
            const size_t REMAIN = swath.data.size() - OFFSET;
            if (REMAIN < len) {
                continue;
            }
            // Assemble bytes
            out.resize(len);
            for (size_t i = 0; i < len; ++i) {
                out[i] = swath.data[OFFSET + i].oldByte;
            }
            return true;
        }
        return false;
    }
};

}  // namespace scan
