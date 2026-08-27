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
#include "newscanmem/scan/types.hpp"
#include "newscanmem/ui/show_message.hpp"

namespace core {

/**
 * @brief Format value according to data type
 * @param valueBytes Raw byte sequence
 * @param dataType Scan data type
 * @param bigEndian Whether to display as big endian
 * @return Formatted string
 */
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
[[nodiscard]] auto formatValueByType(
    const std::vector<std::uint8_t>& valueBytes,
    std::optional<ScanDataType> dataType, bool bigEndian) -> std::string;

/**
 * @struct FormatOptions
 * @brief Options for match formatting and display
 */
struct FormatOptions {
    bool showRegion = true;
    bool showIndex = true;
    bool bigEndianDisplay = false;
    std::optional<ScanDataType> dataType;
};

/**
 * @class MatchFormatter
 * @brief Formats and displays match entries
 */
class MatchFormatter {
   public:
    [[nodiscard]] static auto format(
        const std::vector<MatchEntry>& entries, std::size_t totalCount,
        const FormatOptions& options = {}) -> std::vector<std::string>;
};

}  // namespace core
