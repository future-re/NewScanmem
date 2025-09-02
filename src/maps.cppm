module;

#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

export module maps;

enum class RegionType : uint8_t { MISC, EXE, CODE, HEAP, STACK };

/*
 * Region Type Descriptions:
 * - misc:   未知
 * - exe:    主程序代码段
 * - code:   动态库或者其他代码段
 * - heap:   堆内存区域
 * - stack:  栈内存区域
 */
constexpr std::array<std::string_view, 5> REGION_TYPE_NAMES = {
    "misc", "exe", "code", "heap", "stack"};

/*
 * RegionScanLevel Descriptions:
 * - ALL:                       扫描所有内存区域
 * - ALL_RW:                    扫描所有可读写的内存区域
 * - HEAP_STACK_EXECUTABLE:     仅扫描堆、栈和可执行区域
 * - HEAP_STACK_EXECUTABLE_BSS: 扫描堆、栈、可执行区域以及 BSS 段
 */
enum class RegionScanLevel : uint8_t {
    ALL,
    ALL_RW,
    HEAP_STACK_EXECUTABLE,
    HEAP_STACK_EXECUTABLE_BSS
};

/*
 * RegionFlags:
 * - read:     是否具有读取权限
 * - write:    是否具有写入权限
 * - exec:     是否具有执行权限
 * - shared:   是否为共享内存区域
 * - private_: 是否为私有内存区域
 */
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
    RegionType type{RegionType::MISC};
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
        auto* startBytes = static_cast<std::byte*>(start);
        return address >= start && address < (startBytes + size);
    }
};

class MapsReader {
   public:
    struct Error {
        std::string message;
        std::error_code code;
    };

    [[nodiscard]] static auto readProcessMaps(
        pid_t pid, RegionScanLevel level = RegionScanLevel::ALL)
        -> std::expected<std::vector<Region>, Error> {
        auto mapsPath =
            std::filesystem::path{"/proc"} / std::to_string(pid) / "maps";
        auto exePath =
            std::filesystem::path{"/proc"} / std::to_string(pid) / "exe";

        if (!std::filesystem::exists(mapsPath)) {
            return std::unexpected(
                Error{.message = std::format("Maps file {} does not exist",
                                             mapsPath.string()),
                      .code = std::make_error_code(
                          std::errc::no_such_file_or_directory)});
        }

        std::ifstream mapsFile{mapsPath};
        if (!mapsFile) {
            return std::unexpected(Error{
                .message = std::format("Failed to open maps file {}",
                                       mapsPath.string()),
                .code = std::make_error_code(std::errc::permission_denied)});
        }

        std::string exeName;
        if (std::filesystem::exists(exePath)) {
            try {
                exeName = std::filesystem::read_symlink(exePath).string();
            } catch (const std::filesystem::filesystem_error&) {
                // Ignore errors reading exe symlink
            }
        }

        std::vector<Region> regions;
        std::string line;

        unsigned int codeRegions = 0;
        unsigned int exeRegions = 0;
        unsigned long prevEnd = 0;
        unsigned long loadAddr = 0;
        unsigned long exeLoad = 0;
        bool isExe = false;
        std::string binName;

        while (std::getline(mapsFile, line)) {
            if (auto parsed = parseMapLine(line, exeName, codeRegions,
                                           exeRegions, prevEnd, loadAddr,
                                           exeLoad, isExe, binName, level)) {
                parsed->id = regions.size();
                regions.push_back(std::move(*parsed));
            }
        }

        return regions;
    }

   private:
    static void parseFilenameFromLine(const std::string& line,
                                      std::string& filename) {
        auto spacePos = line.find_last_of(' ');
        if (spacePos != std::string::npos && spacePos + 1 < line.size()) {
            filename = line.substr(spacePos + 1);
        }
    }

    static void updateRegionState(
        unsigned long start, unsigned long end, char exec,
        const std::string& filename, const std::string& exeName,
        unsigned int& codeRegions, unsigned int& exeRegions,
        unsigned long& prevEnd, unsigned long& loadAddr, unsigned long& exeLoad,
        bool& isExe, std::string& binName) {
        if (codeRegions > 0) {
            if (exec == 'x' ||
                (filename != binName &&
                 (!filename.empty() || start != prevEnd)) ||
                codeRegions >= 4) {
                codeRegions = 0;
                isExe = false;
                if (exeRegions > 1) {
                    exeRegions = 0;
                }
            } else {
                codeRegions++;
                if (isExe) {
                    exeRegions++;
                }
            }
        }

        if (codeRegions == 0) {
            if (exec == 'x' && !filename.empty()) {
                codeRegions++;
                if (filename == exeName) {
                    exeRegions = 1;
                    exeLoad = start;
                    isExe = true;
                }
                binName = filename;
            } else if (exeRegions == 1 && !filename.empty() &&
                       filename == exeName) {
                codeRegions = ++exeRegions;
                loadAddr = exeLoad;
                isExe = true;
                binName = filename;
            }
            if (exeRegions < 2) {
                loadAddr = start;
            }
        }

        prevEnd = end;
    }

    static auto determineRegionType(bool isExe, unsigned int codeRegions,
                                    const std::string& filename) -> RegionType {
        if (isExe) {
            return RegionType::EXE;
        }
        if (codeRegions > 0) {
            return RegionType::CODE;
        }
        if (filename == "[heap]") {
            return RegionType::HEAP;
        }
        if (filename == "[stack]") {
            return RegionType::STACK;
        }
        return RegionType::MISC;
    }

    static auto regionUsefulForLevel(RegionType type,
                                     const std::string& filename,
                                     const std::string& exeName,
                                     RegionScanLevel level) -> bool {
        switch (level) {
            case RegionScanLevel::ALL:
            case RegionScanLevel::ALL_RW:
                return true;
            case RegionScanLevel::HEAP_STACK_EXECUTABLE_BSS:
                if (filename.empty()) {
                    return true;
                }
                [[fallthrough]];
            case RegionScanLevel::HEAP_STACK_EXECUTABLE:
                if (type == RegionType::HEAP || type == RegionType::STACK) {
                    return true;
                }
                return type == RegionType::EXE || filename == exeName;
        }
        return false;
    }

    static auto parseMapLine(const std::string& line,
                             const std::string& exeName,
                             unsigned int& codeRegions,
                             unsigned int& exeRegions, unsigned long& prevEnd,
                             unsigned long& loadAddr, unsigned long& exeLoad,
                             bool& isExe, std::string& binName,
                             RegionScanLevel level) -> std::optional<Region> {
        unsigned long start = 0;
        unsigned long end = 0;
        char read = 0;
        char write = 0;
        char exec = 0;
        char cow = 0;
        int offset = 0;
        int devMajor = 0;
        int devMinor = 0;
        int inode = 0;
        std::string fnameBuf;

        std::istringstream iss(line);
        iss >> std::hex >> start;
        iss.ignore(1, '-');
        iss >> std::hex >> end;
        iss >> read >> write >> exec >> cow;
        iss >> std::hex >> offset;
        iss >> devMajor;
        iss.ignore(1, ':');
        iss >> devMinor;
        iss >> inode;
        iss >> fnameBuf;

        if (iss.fail()) {
            return std::nullopt;
        }

        std::string filename = fnameBuf;

        // Read the actual filename from the line (override if present)
        parseFilenameFromLine(line, filename);

        // Update code/exe detection state
        updateRegionState(start, end, exec, filename, exeName, codeRegions,
                          exeRegions, prevEnd, loadAddr, exeLoad, isExe,
                          binName);

        // Must have read permissions and non-zero size
        if (read != 'r' || (end - start) == 0) {
            return std::nullopt;
        }

        // Determine region type
        RegionType type = determineRegionType(isExe, codeRegions, filename);

        // Check if region is useful based on scan level
        if (!regionUsefulForLevel(type, filename, exeName, level)) {
            return std::nullopt;
        }

        // Skip non-writable regions for non-ALL scan levels
        if (level != RegionScanLevel::ALL && write != 'w') {
            return std::nullopt;
        }

        Region result;
        result.start = std::bit_cast<void*>(start);
        result.size = end - start;
        result.type = type;
        result.loadAddr = std::bit_cast<void*>(loadAddr);
        result.filename = filename;

        result.flags.read = true;
        result.flags.write = (write == 'w');
        result.flags.exec = (exec == 'x');
        result.flags.shared = (cow == 's');
        result.flags.exclusive = (cow == 'p');

        return result;
    }
};

// Convenience function for direct usage
[[nodiscard]] inline auto readProcessMaps(
    pid_t pid, RegionScanLevel level = RegionScanLevel::ALL)
    -> std::expected<std::vector<Region>, MapsReader::Error> {
    return MapsReader::readProcessMaps(pid, level);
}
