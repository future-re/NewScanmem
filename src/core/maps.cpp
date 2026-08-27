#include "newscanmem/core/maps.hpp"

#include <bit>
#include <filesystem>
#include <format>
#include <fstream>
#include <optional>
#include <sstream>

namespace core {
namespace {
struct ParseState { unsigned int code_regions{}; unsigned int exe_regions{}; unsigned long previous_end{}; unsigned long load_address{}; unsigned long executable_load{}; bool is_executable{}; std::string binary_name; };
}  // namespace
auto MapsReader::parseAll(std::istream& stream, const std::string& executable, const RegionScanLevel level) -> std::vector<Region> { std::vector<Region> result; std::string line; ParseState state; while (std::getline(stream, line)) { if (auto region = parseMapLine(line, executable, state.code_regions, state.exe_regions, state.previous_end, state.load_address, state.executable_load, state.is_executable, state.binary_name, level)) { region->id = result.size(); result.push_back(std::move(*region)); } } return result; }
auto MapsReader::readProcessMaps(const pid_t pid, const RegionScanLevel level) -> std::expected<std::vector<Region>, Error> {
  const auto maps_path = std::filesystem::path{"/proc"} / std::to_string(pid) / "maps"; const auto executable_path = std::filesystem::path{"/proc"} / std::to_string(pid) / "exe";
  if (!std::filesystem::exists(maps_path)) return std::unexpected(Error{.message = std::format("Maps file {} does not exist", maps_path.string()), .code = std::make_error_code(std::errc::no_such_file_or_directory)});
  std::ifstream maps{maps_path}; if (!maps) return std::unexpected(Error{.message = std::format("Failed to open maps file {}", maps_path.string()), .code = std::make_error_code(std::errc::permission_denied)});
  std::string executable; if (std::filesystem::exists(executable_path)) try { executable = std::filesystem::read_symlink(executable_path).string(); } catch (const std::filesystem::filesystem_error&) {}
  return parseAll(maps, executable, level);
}
#if !defined(NDEBUG) || defined(ENABLE_TEST_API)
auto MapsReader::parseMapsFromStream(std::istream& stream, const std::string& executable, const RegionScanLevel level) -> std::vector<Region> { return parseAll(stream, executable, level); }
#endif
void MapsReader::updateRegionState(const unsigned long start, const unsigned long end, const char executable, const std::string& filename, const std::string& exe_name, unsigned int& code_regions, unsigned int& exe_regions, unsigned long& previous_end, unsigned long& load_address, unsigned long& executable_load, bool& is_executable, std::string& binary_name) {
  if (code_regions > 0) { if (executable == 'x' || (filename != binary_name && (!filename.empty() || start != previous_end)) || code_regions >= 4) { code_regions = 0; is_executable = false; if (exe_regions > 1) exe_regions = 0; } else { ++code_regions; if (is_executable) ++exe_regions; } }
  if (code_regions == 0) { if (executable == 'x' && !filename.empty()) { ++code_regions; if (filename == exe_name) { exe_regions = 1; executable_load = start; is_executable = true; } binary_name = filename; } else if (exe_regions == 1 && !filename.empty() && filename == exe_name) { code_regions = ++exe_regions; load_address = executable_load; is_executable = true; binary_name = filename; } if (exe_regions < 2) load_address = start; }
  previous_end = end;
}
auto MapsReader::determineRegionType(const bool executable, const unsigned int code_regions, const std::string& filename) -> RegionType { if (executable) return RegionType::EXE; if (code_regions > 0) return RegionType::CODE; if (filename == "[heap]") return RegionType::HEAP; if (filename == "[stack]") return RegionType::STACK; return RegionType::UNKNOW; }
auto MapsReader::regionUsefulForLevel(const RegionType type, const std::string& filename, const std::string& executable, const RegionScanLevel level) -> bool { switch (level) { case RegionScanLevel::ALL: case RegionScanLevel::ALL_RW: return true; case RegionScanLevel::HEAP_STACK_EXECUTABLE_BSS: if (filename.empty()) return true; [[fallthrough]]; case RegionScanLevel::HEAP_STACK_EXECUTABLE: return type == RegionType::HEAP || type == RegionType::STACK || type == RegionType::EXE || filename == executable; } return false; }
auto MapsReader::parseMapLine(const std::string& line, const std::string& executable, unsigned int& code_regions, unsigned int& exe_regions, unsigned long& previous_end, unsigned long& load_address, unsigned long& executable_load, bool& is_executable, std::string& binary_name, const RegionScanLevel level) -> std::optional<Region> {
  unsigned long start{}, end{}; char read{}, write{}, execute{}, cow{}; int offset{}, major{}, minor{}, inode{}; std::istringstream input{line}; input >> std::hex >> start; input.ignore(1, '-'); input >> std::hex >> end >> read >> write >> execute >> cow >> std::hex >> offset >> major; input.ignore(1, ':'); input >> minor >> inode;
  std::string filename; if (!input.eof()) { std::getline(input, filename); const auto first = filename.find_first_not_of(' '); if (first == std::string::npos) filename.clear(); else filename.erase(0, first); } if (input.fail()) return std::nullopt;
  updateRegionState(start, end, execute, filename, executable, code_regions, exe_regions, previous_end, load_address, executable_load, is_executable, binary_name); if (read != 'r' || end == start) return std::nullopt; const auto type = determineRegionType(is_executable, code_regions, filename); if (!regionUsefulForLevel(type, filename, executable, level) || (level != RegionScanLevel::ALL && write != 'w')) return std::nullopt;
  Region result; result.start = std::bit_cast<void*>(start); result.size = end - start; result.type = type; result.loadAddr = std::bit_cast<void*>(load_address); result.filename = std::move(filename); result.flags = {.read = true, .write = write == 'w', .exec = execute == 'x', .shared = cow == 's', .exclusive = cow == 'p'}; return result;
}
auto readProcessMaps(const pid_t pid, const RegionScanLevel level) -> std::expected<std::vector<Region>, MapsReader::Error> { return MapsReader::readProcessMaps(pid, level); }
}  // namespace core
