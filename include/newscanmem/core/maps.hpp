#pragma once

#include <sys/types.h>

#include <array>
#include <bit>
#include <cstdint>
#include <expected>
#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace core {
enum class RegionType : std::uint8_t { UNKNOW, EXE, CODE, HEAP, STACK };
constexpr std::array<std::string_view, 5> REGION_TYPE_NAMES{
    "unknow", "exe", "code", "heap", "stack"};
enum class RegionScanLevel : std::uint8_t {
    ALL,
    ALL_RW,
    HEAP_STACK_EXECUTABLE,
    HEAP_STACK_EXECUTABLE_BSS
};
struct RegionFlags {
    bool read : 1;
    bool write : 1;
    bool exec : 1;
    bool shared : 1;
    bool exclusive : 1;
};
struct Region {
    void* start{};
    std::size_t size{};
    RegionType type{RegionType::UNKNOW};
    RegionFlags flags{};
    void* loadAddr{};
    std::string filename;
    std::size_t id{};
    [[nodiscard]] auto isReadable() const noexcept -> bool {
        return flags.read;
    }
    [[nodiscard]] auto isWritable() const noexcept -> bool {
        return flags.write;
    }
    [[nodiscard]] auto isExecutable() const noexcept -> bool {
        return flags.exec;
    }
    [[nodiscard]] auto isShared() const noexcept -> bool {
        return flags.shared;
    }
    [[nodiscard]] auto isPrivate() const noexcept -> bool {
        return flags.exclusive;
    }
    [[nodiscard]] auto asSpan() const noexcept
        -> std::pair<void*, std::size_t> {
        return {start, size};
    }
    [[nodiscard]] auto contains(void* address) const noexcept -> bool {
        const auto begin = std::bit_cast<std::uintptr_t>(start);
        const auto value = std::bit_cast<std::uintptr_t>(address);
        return value >= begin && value - begin < size;
    }
};
class MapsReader {
   public:
    struct Error {
        std::string message;
        std::error_code code;
    };
    [[nodiscard]] static auto readProcessMaps(
        pid_t pid, RegionScanLevel level = RegionScanLevel::ALL_RW)
        -> std::expected<std::vector<Region>, Error>;
#if !defined(NDEBUG) || defined(ENABLE_TEST_API)
    [[nodiscard]] static auto parseMapsFromStream(
        std::istream& stream, const std::string& exe_name,
        RegionScanLevel level = RegionScanLevel::ALL) -> std::vector<Region>;
#endif
   private:
    [[nodiscard]] static auto parseAll(
        std::istream& stream, const std::string& executable,
        RegionScanLevel level) -> std::vector<Region>;
    static void updateRegionState(
        unsigned long start, unsigned long end, char exec,
        const std::string& filename, const std::string& exe_name,
        unsigned int& code_regions, unsigned int& exe_regions,
        unsigned long& previous_end, unsigned long& load_address,
        unsigned long& executable_load, bool& is_executable,
        std::string& binary_name);
    [[nodiscard]] static auto determineRegionType(
        bool is_executable, unsigned int code_regions,
        const std::string& filename) -> RegionType;
    [[nodiscard]] static auto regionUsefulForLevel(
        RegionType type, const std::string& filename,
        const std::string& exe_name, RegionScanLevel level) -> bool;
    [[nodiscard]] static auto parseMapLine(
        const std::string& line, const std::string& exe_name,
        unsigned int& code_regions, unsigned int& exe_regions,
        unsigned long& previous_end, unsigned long& load_address,
        unsigned long& executable_load, bool& is_executable,
        std::string& binary_name,
        RegionScanLevel level) -> std::optional<Region>;
};
[[nodiscard]] auto readProcessMaps(pid_t pid,
                                   RegionScanLevel level = RegionScanLevel::ALL)
    -> std::expected<std::vector<Region>, MapsReader::Error>;
}  // namespace core
