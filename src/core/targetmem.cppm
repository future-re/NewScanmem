/**
 * @file targetmem.cppm
 * @brief Target memory management: match tracking and old value storage
 * (Target memory management: match tracking and history storage)
 *
 * Provides data structures for storing scan results (matches) and historical
 * byte values from target process memory regions (swaths). Used by scanner to
 * track matching addresses across multiple scan iterations.
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

export module core.targetmem;

import value.flags;
import ui.show_message;

// Interface: exported types and functions
export {
    /**
     * @struct RangeMark
     * @brief Marks a contiguous range in swath.data with match flags
     */
    struct RangeMark {
        size_t startIndex;  ///< Start index in swath.data
        size_t length;      ///< Number of bytes in range
        MatchFlags flags;   ///< Match flags for this range
    };

    /**
     * @struct OldValueAndMatchInfo
     * @brief Stores one byte's historical value and its match status
     */
    struct OldValueAndMatchInfo {
        uint8_t oldValue;      ///< Historical byte value
        MatchFlags matchInfo;  ///< Match flags for this byte
    };

    class MatchesAndOldValuesSwath {
       public:
        void* firstByteInChild = nullptr;
        std::vector<OldValueAndMatchInfo> data;
        // Record flags per range (helper structure for UI or post-processing)
        std::vector<RangeMark> rangeMarks;

        MatchesAndOldValuesSwath() = default;

        /*
         * addElement(addr, byte, matchFlags)
         *
         * Append one byte's history value and match flags to swath.
         * Convention: each element in data strictly corresponds to 1 byte
         * offset in target process. Thus we can reconstruct target address via
         * `firstByteInChild + index`.
         *
         * - addr: 该字节在目标进程中的起始地址（字节地址）。
         * - byte: 读取到的字节值。
         * - matchFlags: 该字节的匹配状态（MatchFlags）。
         *
         * 语义:
         *  - 如果 data 为空，则将 firstByteInChild 设为 addr，之后按顺序
         * push_back。
         *  - 要求调用者按地址顺序添加字节以保持 firstByteInChild 与 data
         * 的对应关系。
         */
        void addElement(void* addr, uint8_t byte, MatchFlags matchFlags) {
            if (data.empty()) {
                firstByteInChild = addr;
            }
            data.push_back({byte, matchFlags});
        }

        // Append bytes in batch
        void appendRange(void* baseAddr, const uint8_t* bytes, size_t length,
                         MatchFlags initial = MatchFlags::EMPTY) {
            if (length == 0) {
                return;
            }
            if (data.empty()) {
                firstByteInChild = baseAddr;
            }
            for (std::size_t i = 0; i < length; ++i) {
                data.push_back({bytes[i], initial});
            }
        }

        // Batch mark by index range [startIndex, startIndex+length)
        void markRangeByIndex(size_t startIndex, size_t length,
                              MatchFlags flags) {
            if (length == 0 || startIndex >= data.size()) {
                return;
            }
            size_t endIndex = std::min(startIndex + length, data.size());
            for (size_t i = startIndex; i < endIndex; ++i) {
                data[i].matchInfo = data[i].matchInfo | flags;
            }
            rangeMarks.push_back({startIndex, endIndex - startIndex, flags});
        }

        // Batch mark by target address range [addr, addr+length)
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

        /*
         * toPrintableString(idx, len)
         *
         * 将从 idx 起最多 len
         * 个字节转换为用于显示的可打印字符序列（不可打印字符用 '.' 代替）。
         * 返回一个 std::string，方便在 UI/log 中显示区域内的文本表示。
         */
        [[nodiscard]] auto toPrintableString(size_t idx, size_t len) const
            -> std::string {
            std::ostringstream oss;
            size_t count = std::min(len, data.size() - idx);
            for (size_t i = 0; i < count; ++i) {
                auto byteVal = data[idx + i].oldValue;
                oss << ((std::isprint(byteVal) != 0)
                            ? static_cast<char>(byteVal)
                            : '.');
            }
            return oss.str();
        }

        /*
         * toByteArrayText(idx, len)
         *
         * 将从 idx 起最多 len
         * 个字节转换为十六进制字符串（以空格分隔），便于调试查看原始字节。
         */
        [[nodiscard]] auto toByteArrayText(size_t idx, size_t len) const
            -> std::string {
            std::ostringstream oss;
            size_t count = std::min(len, data.size() - idx);
            oss << std::nouppercase << std::hex;
            for (size_t i = 0; i < count; ++i) {
                oss << std::setw(2) << std::setfill('0')
                    << (static_cast<int>(data[idx + i].oldValue) & 0xFF);
                if (i + 1 < count) {
                    oss << ' ';
                }
            }
            return oss.str();
        }
    };

    /*
     * MatchesAndOldValuesArray
     *
     * 管理多个 swath（连续字节片段）的集合，用于保存目标进程中多个区域的
     * 历史字节值与匹配信息。
     *
     * 字段:
     *  - maxNeededBytes:
     为了预分配或限制使用的最大字节数（语义由调用方定义）。
     *  - swaths: 各个 MatchesAndOldValuesSwath 实例的容器。
     */
    class MatchesAndOldValuesArray {
       public:
        std::vector<MatchesAndOldValuesSwath> swaths;
        MatchesAndOldValuesArray() = default;

        /*
         * addSwath(swath)
         *
         * Append a complete swath to the array end.
         * Semantics: copy the swath (caller may continue using the original);
         * keep order consistent with address order (if caller guarantees).
         */
        void addSwath(const MatchesAndOldValuesSwath& swath) {
            swaths.push_back(swath);
        }

        /*
         * nthMatch(n)
         *
         * Return the n-th position marked as matched (matchInfo != EMPTY).
         *
         * Returns:
         *  - On success: std::pair<swath_ptr, index>, where swath_ptr points
         *    to the MatchesAndOldValuesSwath containing the match and index is
         *    the position within swath.data.
         *  - On failure: std::nullopt (when n exceeds total matches).
         */
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

        /*
         * deleteInAddressRange(start, end, numMatches)
         *
         * Delete all recorded bytes within the address range [start, end)
         * (inclusive of start, exclusive of end), including matches and
         * non-matches.
         *
         * Parameters:
         *  - start, end: address half-open interval to delete.
         *  - numMatches: output parameter; number of deleted elements that were
         *    marked as matches (matchInfo != EMPTY).
         *
         * Logic:
         *  - Iterate each swath's data (vector of OldValueAndMatchInfo per
         * byte).
         *  - Compute each element's process address by index:
         *      addr = firstByteInChild + index
         *    using &info - swath.data.data() to obtain the element index.
         *  - If addr falls within [start, end), delete the element and
         * increment numMatches when it was a match.
         *  - Use std::ranges::remove_if followed by erase to efficiently drop
         *    elements.
         */
        void deleteInAddressRange(void* start, void* end,
                                  unsigned long& numMatches) {
            numMatches = 0;
            for (auto& swath : swaths) {
                auto iter = std::ranges::remove_if(
                    swath.data, [&](const OldValueAndMatchInfo& info) {
                        void* addr =
                            static_cast<char*>(swath.firstByteInChild) +
                            (&info - swath.data.data());
                        if (addr >= start && addr < end) {
                            if (info.matchInfo != MatchFlags::EMPTY) {
                                ++numMatches;
                            }
                            return true;
                        }
                        return false;
                    });
                swath.data.erase(iter.begin(), swath.data.end());
                // Also clear rangeMarks fully covered by deletion range
                // (conservative)
                auto riter = std::ranges::remove_if(
                    swath.rangeMarks, [&](const RangeMark& mark) {
                        void* rstart =
                            static_cast<char*>(swath.firstByteInChild) +
                            mark.startIndex;
                        void* rend = static_cast<char*>(rstart) + mark.length;
                        return (rstart >= start && rend <= end);
                    });
                swath.rangeMarks.erase(riter.begin(), swath.rangeMarks.end());
            }
        }

        /*
         * getRawBytesAt(addr, len, out)
         *
         * Try to assemble a contiguous slice of historical bytes starting at
         * target process address `addr` with length `len`. On success, writes
         * bytes into `out` and returns true.
         */
        [[nodiscard]] auto getRawBytesAt(void* addr, size_t len,
                                         std::vector<uint8_t>& out) const
            -> bool {
            if (addr == nullptr || len == 0) {
                return false;
            }
            for (const auto& swath : swaths) {
                if (swath.firstByteInChild == nullptr || swath.data.empty()) {
                    continue;
                }
                const auto* base =
                    static_cast<const char*>(swath.firstByteInChild);
                const auto* curr = static_cast<const char*>(addr);
                if (curr < base) {
                    continue;
                }
                const size_t OFFSET = static_cast<size_t>(curr - base);
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
                    out[i] = swath.data[OFFSET + i].oldValue;
                }
                return true;
            }
            return false;
        }
    };
}
