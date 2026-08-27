#include "newscanmem/core/match_formatter.hpp"

namespace core {
namespace {
auto hexBytes(const std::vector<std::uint8_t>& bytes) -> std::string {
    std::string result;
    for (const auto byte : bytes) {
        if (!result.empty()) result += " ";
        result += std::format("0x{:02x}", static_cast<unsigned>(byte));
    }
    return result;
}
}  // namespace
auto formatValueByType(const std::vector<std::uint8_t>& bytes,
                       const std::optional<ScanDataType> type,
                       const bool big_endian) -> std::string {
    if (!type || bytes.empty()) {
        const auto result = hexBytes(bytes);
        return result.empty() ? "0x00" : result;
    }
    const auto read_value = [&bytes, big_endian]<typename T>() {
        T value{};
        if (bytes.size() < sizeof(T)) return value;
        std::memcpy(&value, bytes.data(), sizeof(T));
        if (big_endian != (std::endian::native == std::endian::big)) {
            if constexpr (std::is_integral_v<T>)
                value = std::byteswap(value);
            else
                std::reverse(
                    reinterpret_cast<std::uint8_t*>(&value),
                    reinterpret_cast<std::uint8_t*>(&value) + sizeof(T));
        }
        return value;
    };
    switch (*type) {
        case ScanDataType::INTEGER_8:
            return std::format("{}", static_cast<std::int8_t>(bytes[0]));
        case ScanDataType::INTEGER_16:
            return std::format("{}",
                               read_value.template operator()<std::int16_t>());
        case ScanDataType::INTEGER_32:
            return std::format("{}",
                               read_value.template operator()<std::int32_t>());
        case ScanDataType::INTEGER_64:
            return std::format("{}",
                               read_value.template operator()<std::int64_t>());
        case ScanDataType::FLOAT_32:
            return std::format("{:.6g}",
                               read_value.template operator()<float>());
        case ScanDataType::FLOAT_64:
            return std::format("{:.15g}",
                               read_value.template operator()<double>());
        case ScanDataType::STRING:
            return {bytes.begin(), bytes.end()};
        default:
            return hexBytes(bytes);
    }
}
auto MatchFormatter::format(
    const std::vector<MatchEntry>& entries, const std::size_t total,
    const FormatOptions& options) -> std::vector<std::string> {
    std::vector<std::string> lines;
    if (options.showIndex && options.showRegion) {
        lines.emplace_back("Index  Address             Value     Region");
        lines.emplace_back("-----------------------------------------------");
    } else if (options.showIndex) {
        lines.emplace_back("Index  Address             Value");
        lines.emplace_back("---------------------------------------");
    } else if (options.showRegion) {
        lines.emplace_back("Address             Value     Region");
        lines.emplace_back("---------------------------------------");
    } else {
        lines.emplace_back("Address             Value");
        lines.emplace_back("-----------------------------------");
    }
    for (const auto& entry : entries) {
        const auto value = formatValueByType(entry.value, options.dataType,
                                             options.bigEndianDisplay);
        const auto region = std::format("[{}]", entry.region);
        if (options.showIndex && options.showRegion)
            lines.emplace_back(std::format("{:<6} 0x{:016x}  {:<12} {}",
                                           entry.index, entry.address, value,
                                           region));
        else if (options.showIndex)
            lines.emplace_back(std::format("{:<6} 0x{:016x}  {}", entry.index,
                                           entry.address, value));
        else if (options.showRegion)
            lines.emplace_back(std::format("0x{:016x}  {:<12} {}",
                                           entry.address, value, region));
        else
            lines.emplace_back(
                std::format("0x{:016x}  {}", entry.address, value));
    }
    if (total > entries.size())
        lines.emplace_back(std::format("\n... and {} more matches (total: {})",
                                       total - entries.size(), total));
    lines.emplace_back(
        std::format("\nShowing {} of {} matches", entries.size(), total));
    return lines;
}
}  // namespace core
