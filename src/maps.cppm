module;

#include <array>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>

export module maps;

enum class RegionType : uint8_t {
    MISC,
    EXE,
    CODE,
    HEAP,
    STACK
};

constexpr std::array<std::string_view, 5> REGION_TYPE_NAMES = {
    "misc", "exe", "code", "heap", "stack"
};

enum class RegionScanLevel : uint8_t {
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
    bool private_ : 1;
};

struct Region {
    void* start;
    std::size_t size;
    RegionType type;
    RegionFlags flags;
    void* loadAddr;
    std::string filename;
    std::size_t id;
    
    [[nodiscard]] bool isReadable() const noexcept { return flags.read; }
    [[nodiscard]] bool isWritable() const noexcept { return flags.write; }
    [[nodiscard]] bool isExecutable() const noexcept { return flags.exec; }
    [[nodiscard]] bool isShared() const noexcept { return flags.shared; }
    [[nodiscard]] bool isPrivate() const noexcept { return flags.private_; }
    
    [[nodiscard]] std::pair<void*, std::size_t> asSpan() const noexcept {
        return {start, size};
    }
    
    [[nodiscard]] bool contains(void* address) const noexcept {
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
    
    [[nodiscard]] static std::expected<std::vector<Region>, Error> 
    readProcessMaps(pid_t pid, RegionScanLevel level = RegionScanLevel::ALL) {
        auto mapsPath = std::filesystem::path{"/proc"} / std::to_string(pid) / "maps";
        auto exePath = std::filesystem::path{"/proc"} / std::to_string(pid) / "exe";
        
        if (!std::filesystem::exists(mapsPath)) {
            return std::unexpected(Error{
                std::format("Maps file {} does not exist", mapsPath.string()),
                std::make_error_code(std::errc::no_such_file_or_directory)
            });
        }
        
        std::ifstream mapsFile{mapsPath};
        if (!mapsFile) {
            return std::unexpected(Error{
                std::format("Failed to open maps file {}", mapsPath.string()),
                std::make_error_code(std::errc::permission_denied)
            });
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
    static std::optional<Region> parseMapLine(
        const std::string& line,
        const std::string& exeName,
        unsigned int& codeRegions,
        unsigned int& exeRegions,
        unsigned long& prevEnd,
        unsigned long& loadAddr,
        unsigned long& exeLoad,
        bool& isExe,
        std::string& binName,
        RegionScanLevel level) {
        
        unsigned long start, end;
        char read, write, exec, cow;
        int offset, devMajor, devMinor, inode;
        std::string filename;
        
        if (std::sscanf(line.c_str(), "%lx-%lx %c%c%c%c %x %x:%x %d %255s",
                       &start, &end, &read, &write, &exec, &cow, 
                       &offset, &devMajor, &devMinor, &inode, 
                       filename.data()) < 6) {
            return std::nullopt;
        }
        
        // Fix filename after sscanf
        filename.resize(strlen(filename.c_str()));
        
        // Read the actual filename from the line
        auto space_pos = line.find_last_of(' ');
        if (space_pos != std::string::npos && space_pos + 1 < line.size()) {
            filename = line.substr(space_pos + 1);
        }
        
        // Detect further regions of the same ELF file
        if (codeRegions > 0) {
            if (exec == 'x' || 
                (filename != binName && (!filename.empty() || start != prevEnd)) ||
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
        
        // Must have read permissions and non-zero size
        if (read != 'r' || (end - start) == 0) {
            return std::nullopt;
        }
        
        // Determine region type
        RegionType type = RegionType::MISC;
        if (isExe) {
            type = RegionType::EXE;
        } else if (codeRegions > 0) {
            type = RegionType::CODE;
        } else if (filename == "[heap]") {
            type = RegionType::HEAP;
        } else if (filename == "[stack]") {
            type = RegionType::STACK;
        }
        
        // Check if region is useful based on scan level
        bool useful = false;
        switch (level) {
            case RegionScanLevel::ALL:
                useful = true;
                break;
            case RegionScanLevel::ALL_RW:
                useful = true;
                break;
            case RegionScanLevel::HEAP_STACK_EXECUTABLE_BSS:
                if (filename.empty()) {
                    useful = true;
                    break;
                }
                [[fallthrough]];
            case RegionScanLevel::HEAP_STACK_EXECUTABLE:
                if (type == RegionType::HEAP || type == RegionType::STACK) {
                    useful = true;
                    break;
                }
                if (type == RegionType::EXE || filename == exeName) {
                    useful = true;
                }
                break;
        }
        
        if (!useful) {
            return std::nullopt;
        }
        
        // Skip non-writable regions for non-ALL scan levels
        if (level != RegionScanLevel::ALL && write != 'w') {
            return std::nullopt;
        }
        
        Region result;
        result.start = reinterpret_cast<void*>(start);
        result.size = end - start;
        result.type = type;
        result.loadAddr = reinterpret_cast<void*>(loadAddr);
        result.filename = filename;
        
        result.flags.read = true;
        result.flags.write = (write == 'w');
        result.flags.exec = (exec == 'x');
        result.flags.shared = (cow == 's');
        result.flags.private_ = (cow == 'p');
        
        return result;
    }
};

// Convenience function for direct usage
[[nodiscard]] inline std::expected<std::vector<Region>, MapsReader::Error> 
readProcessMaps(pid_t pid, RegionScanLevel level = RegionScanLevel::ALL) {
    return MapsReader::readProcessMaps(pid, level);
}

