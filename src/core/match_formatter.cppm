/**
 * @file match_formatter.cppm
 * @brief Match result formatting and display
 */

module;

#include <algorithm>
#include <bit>
#include <cstdint>
#include <cstring>
#include <format>
#include <optional>
#include <vector>

export module core.match_formatter;

import core.match;
import ui.show_message;
import scan.types;

export namespace core {

/**
 * @brief Format value according to data type
 * @param valueBytes Raw byte sequence
 * @param dataType Scan data type
 * @param bigEndian Whether to display as big endian
 * @return Formatted string
 */
// NOLINTNEXTLINE
[[nodiscard]] inline auto formatValueByType(
    const std::vector<std::uint8_t>& valueBytes,
    std::optional<ScanDataType> dataType, bool bigEndian) -> std::string {
    // If no data type specified or empty bytes, show as hex bytes
    if (!dataType || valueBytes.empty()) {
        std::string result;
        for (auto byte : valueBytes) {
            if (!result.empty()) {
                result += " ";
            }
            result += std::format("0x{:02x}", static_cast<unsigned>(byte));
        }
        return result.empty() ? "0x00" : result;
    }

    // Helper lambda to read multi-byte value with endianness
    auto readValue = [&valueBytes, bigEndian]<typename T>() -> T {
        if (valueBytes.size() < sizeof(T)) {
            return T{};  // Not enough bytes
        }
        T value;
        std::memcpy(&value, valueBytes.data(), sizeof(T));

        // Handle endianness for integral types only
        if constexpr (std::is_integral_v<T>) {
            if (bigEndian != (std::endian::native == std::endian::big)) {
                value = std::byteswap(value);
            }
        } else {
            // For floating-point types, swap bytes manually if needed
            if (bigEndian != (std::endian::native == std::endian::big)) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
                auto* bytes = reinterpret_cast<uint8_t*>(&value);
                std::reverse(bytes, bytes + sizeof(T));
            }
        }

        return value;
    };  // Format based on data type
    switch (*dataType) {
        case ScanDataType::INTEGER_8:
            return std::format("{}", static_cast<int8_t>(valueBytes[0]));

        case ScanDataType::INTEGER_16:
            return std::format("{}", readValue.template operator()<int16_t>());

        case ScanDataType::INTEGER_32:
            return std::format("{}", readValue.template operator()<int32_t>());

        case ScanDataType::INTEGER_64:
            return std::format("{}", readValue.template operator()<int64_t>());

        case ScanDataType::FLOAT_32: {
            float val = readValue.template operator()<float>();
            return std::format("{:.6g}", val);
        }

        case ScanDataType::FLOAT_64: {
            auto val = readValue.template operator()<double>();
            return std::format("{:.15g}", val);
        }

        default:
            // For other types, show as hex bytes
            std::string result;
            for (auto byte : valueBytes) {
                if (!result.empty()) {
                    result += " ";
                }
                result += std::format("0x{:02x}", static_cast<unsigned>(byte));
            }
            return result;
    }
}

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
     * @brief Format and display match entries
     * @param entries Match entries to display
     * @param totalCount Total number of matches
     * @param options Formatting options
     */
    static auto display(const std::vector<MatchEntry>& entries,
                        size_t totalCount, const FormatOptions& options = {})
        -> void {
        // Print header
        if (options.showIndex && options.showRegion) {
            ui::MessagePrinter::info(
                "Index  Address             Value     Region");
            ui::MessagePrinter::info(
                "-----------------------------------------------");
        } else if (options.showIndex) {
            ui::MessagePrinter::info("Index  Address             Value");
            ui::MessagePrinter::info("---------------------------------------");
        } else if (options.showRegion) {
            ui::MessagePrinter::info("Address             Value     Region");
            ui::MessagePrinter::info("---------------------------------------");
        } else {
            ui::MessagePrinter::info("Address             Value");
            ui::MessagePrinter::info("-----------------------------------");
        }

        // Print entries
        for (const auto& entry : entries) {
            auto formattedValue = formatValueByType(
                entry.value, options.dataType, options.bigEndianDisplay);

            if (options.showIndex && options.showRegion) {
                ui::MessagePrinter::info(
                    std::format("{:<6} 0x{:016x}  {:<12} {}", entry.index,
                                entry.address, formattedValue, entry.region));
            } else if (options.showIndex) {
                ui::MessagePrinter::info(std::format("{:<6} 0x{:016x}  {}",
                                                     entry.index, entry.address,
                                                     formattedValue));
            } else if (options.showRegion) {
                ui::MessagePrinter::info(
                    std::format("0x{:016x}  {:<12} {}", entry.address,
                                formattedValue, entry.region));
            } else {
                ui::MessagePrinter::info(std::format(
                    "0x{:016x}  {}", entry.address, formattedValue));
            }
        }

        // Print summary
        size_t displayCount = entries.size();
        if (totalCount > displayCount) {
            ui::MessagePrinter::info(
                std::format("\n... and {} more matches (total: {})",
                            totalCount - displayCount, totalCount));
        }

        ui::MessagePrinter::info(std::format("\nShowing {} of {} matches",
                                             displayCount, totalCount));
    }
};

}  // namespace core
