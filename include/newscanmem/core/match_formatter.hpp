#pragma once

/**
 * @file match_formatter.hpp
 * @brief Match result formatting and display
 */

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstring>
#include <format>
#include <optional>
#include <vector>

#include "newscanmem/core/match.hpp"
#include "newscanmem/ui/show_message.hpp"
#include "newscanmem/scan/types.hpp"

namespace core {

/**
 * @brief Format value according to data type
 * @param valueBytes Raw byte sequence
 * @param dataType Scan data type
 * @param bigEndian Whether to display as big endian
 * @return Formatted string
 */
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
[[nodiscard]] auto formatValueByType(const std::vector<std::uint8_t>& value_bytes,
                                     std::optional<ScanDataType> data_type, bool big_endian)
    -> std::string;

/**
 * @struct FormatOptions
 * @brief Options for match formatting and display
 */
struct FormatOptions {
    bool showRegion = true;         // 是否显示区域信息
    bool showIndex = true;          // 是否显示索引
    bool bigEndianDisplay = false;  // 是否按大端显示数值（与读取端序一致）
    std::optional<ScanDataType> dataType;  // 扫描数据类型（用于按类型显示值）
};

/**
 * @class MatchFormatter
 * @brief Formats and displays match entries
 */
class MatchFormatter {
   public:
    /**
     * @brief Format match entries into a list of strings
     * @param entries Match entries to display
     * @param totalCount Total number of matches
     * @param options Formatting options
     * @return Vector of formatted lines
     */
    [[nodiscard]] static auto format(const std::vector<MatchEntry>& entries,
                                     std::size_t total_count, const FormatOptions& options = {})
        -> std::vector<std::string>;
};

}  // namespace core
