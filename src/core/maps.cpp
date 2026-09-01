#include "memseek/core/maps.hpp"

#include <bit>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <sstream>

namespace core {
namespace {
struct ParseState {
    unsigned int codeRegions{};
    unsigned int exeRegions{};
    unsigned long previousEnd{};
    unsigned long loadAddress{};
    unsigned long executableLoad{};
    bool isExecutable{};
    std::string binaryName;
};
}  // namespace

auto MapsReader::parseAll(std::istream& stream, const std::string& executable,
                          const RegionScanLevel level) -> std::vector<Region> {
    std::vector<Region> result;
    std::string line;
    ParseState state;
    while (std::getline(stream, line)) {
        if (auto region = parseMapLine(
                line, executable, state.codeRegions, state.exeRegions,
                state.previousEnd, state.loadAddress, state.executableLoad,
                state.isExecutable, state.binaryName, level)) {
            region->id = result.size();
            result.push_back(std::move(*region));
        }
    }
    return result;
}

auto MapsReader::readProcessMaps(const pid_t pid, const RegionScanLevel level)
    -> std::expected<std::vector<Region>, Error> {
    const auto mapsPath =
        std::filesystem::path{"/proc"} / std::to_string(pid) / "maps";
    const auto executablePath =
        std::filesystem::path{"/proc"} / std::to_string(pid) / "exe";
    if (!std::filesystem::exists(mapsPath))
        return std::unexpected(Error{
            .message = std::format("Maps file {} does not exist", mapsPath.string()),
            .code = std::make_error_code(std::errc::no_such_file_or_directory)});
    std::ifstream maps{mapsPath};
    if (!maps)
        return std::unexpected(Error{
            .message = std::format("Failed to open maps file {}", mapsPath.string()),
            .code = std::make_error_code(std::errc::permission_denied)});
    std::string executable;
    if (std::filesystem::exists(executablePath)) try {
            executable = std::filesystem::read_symlink(executablePath).string();
        } catch (const std::filesystem::filesystem_error&) {
        }
    return parseAll(maps, executable, level);
}

#if !defined(NDEBUG) || defined(ENABLE_TEST_API)
auto MapsReader::parseMapsFromStream(
    std::istream& stream, const std::string& executable,
    const RegionScanLevel level) -> std::vector<Region> {
    return parseAll(stream, executable, level);
}
#endif

void MapsReader::updateRegionState(
    const unsigned long start, const unsigned long end, const char executable,
    const std::string& filename, const std::string& exeName,
    unsigned int& codeRegions, unsigned int& exeRegions,
    unsigned long& previousEnd, unsigned long& loadAddress,
    unsigned long& executableLoad, bool& isExecutable,
    std::string& binaryName) {
    if (codeRegions > 0) {
        if (executable == 'x' ||
            (filename != binaryName && (!filename.empty() || start != previousEnd)) ||
            codeRegions >= 4) {
            codeRegions = 0;
            isExecutable = false;
            if (exeRegions > 1) exeRegions = 0;
        } else {
            ++codeRegions;
            if (isExecutable) ++exeRegions;
        }
    }
    if (codeRegions == 0) {
        if (executable == 'x' && !filename.empty()) {
            ++codeRegions;
            if (filename == exeName) {
                exeRegions = 1;
                executableLoad = start;
                isExecutable = true;
            }
            binaryName = filename;
        } else if (exeRegions == 1 && !filename.empty() && filename == exeName) {
            codeRegions = ++exeRegions;
            loadAddress = executableLoad;
            isExecutable = true;
            binaryName = filename;
        }
        if (exeRegions < 2) loadAddress = start;
    }
    previousEnd = end;
}

auto MapsReader::determineRegionType(
    const bool executable, const unsigned int codeRegions,
    const std::string& filename) -> RegionType {
    if (executable) return RegionType::EXE;
    if (codeRegions > 0) return RegionType::CODE;
    if (filename == "[heap]") return RegionType::HEAP;
    if (filename == "[stack]") return RegionType::STACK;
    return RegionType::UNKNOW;
}

auto MapsReader::regionUsefulForLevel(const RegionType type,
                                      const std::string& filename,
                                      const std::string& executable,
                                      const RegionScanLevel level) -> bool {
    switch (level) {
        case RegionScanLevel::ALL:
        case RegionScanLevel::ALL_RW:
            return true;
        case RegionScanLevel::HEAP_STACK_EXECUTABLE_BSS:
            if (filename.empty()) return true;
            [[fallthrough]];
        case RegionScanLevel::HEAP_STACK_EXECUTABLE:
            return type == RegionType::HEAP || type == RegionType::STACK ||
                   type == RegionType::EXE || filename == executable;
    }
    return false;
}

auto MapsReader::parseMapLine(
    const std::string& line, const std::string& executable,
    unsigned int& codeRegions, unsigned int& exeRegions,
    unsigned long& previousEnd, unsigned long& loadAddress,
    unsigned long& executableLoad, bool& isExecutable,
    std::string& binaryName,
    const RegionScanLevel level) -> std::optional<Region> {
    unsigned long start{}, end{};
    char read{}, write{}, execute{}, cow{};
    int offset{}, major{}, minor{}, inode{};
    std::istringstream input{line};
    input >> std::hex >> start;
    input.ignore(1, '-');
    input >> std::hex >> end >> read >> write >> execute >> cow >> std::hex >>
        offset >> major;
    input.ignore(1, ':');
    input >> minor >> inode;
    std::string filename;
    if (!input.eof()) {
        std::getline(input, filename);
        const auto first = filename.find_first_not_of(' ');
        if (first == std::string::npos)
            filename.clear();
        else
            filename.erase(0, first);
    }
    if (input.fail()) return std::nullopt;
    updateRegionState(start, end, execute, filename, executable, codeRegions,
                      exeRegions, previousEnd, loadAddress, executableLoad,
                      isExecutable, binaryName);
    if (read != 'r' || end == start) return std::nullopt;
    const auto type = determineRegionType(isExecutable, codeRegions, filename);
    if (!regionUsefulForLevel(type, filename, executable, level) ||
        (level != RegionScanLevel::ALL && write != 'w'))
        return std::nullopt;
    Region result;
    result.start = std::bit_cast<void*>(start);
    result.size = end - start;
    result.type = type;
    result.loadAddr = std::bit_cast<void*>(loadAddress);
    result.filename = std::move(filename);
    result.flags = {.read = true,
                    .write = write == 'w',
                    .exec = execute == 'x',
                    .shared = cow == 's',
                    .exclusive = cow == 'p'};
    return result;
}

auto readProcessMaps(const pid_t pid, const RegionScanLevel level)
    -> std::expected<std::vector<Region>, MapsReader::Error> {
    return MapsReader::readProcessMaps(pid, level);
}
}  // namespace core
