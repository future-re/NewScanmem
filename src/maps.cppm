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

enum class region_type : uint8_t {
    misc,
    exe,
    code,
    heap,
    stack
};

constexpr std::array<std::string_view, 5> region_type_names = {
    "misc", "exe", "code", "heap", "stack"
};

enum class region_scan_level : uint8_t {
    all,
    all_rw,
    heap_stack_executable,
    heap_stack_executable_bss
};

struct region_flags {
    bool read : 1;
    bool write : 1;
    bool exec : 1;
    bool shared : 1;
    bool private_ : 1;
};

struct region {
    void* start;
    std::size_t size;
    region_type type;
    region_flags flags;
    void* load_addr;
    std::string filename;
    std::size_t id;
    
    [[nodiscard]] bool is_readable() const noexcept { return flags.read; }
    [[nodiscard]] bool is_writable() const noexcept { return flags.write; }
    [[nodiscard]] bool is_executable() const noexcept { return flags.exec; }
    [[nodiscard]] bool is_shared() const noexcept { return flags.shared; }
    [[nodiscard]] bool is_private() const noexcept { return flags.private_; }
    
    [[nodiscard]] std::pair<void*, std::size_t> as_span() const noexcept {
        return {start, size};
    }
    
    [[nodiscard]] bool contains(void* address) const noexcept {
        auto* start_bytes = static_cast<std::byte*>(start);
        return address >= start && address < (start_bytes + size);
    }
};

class maps_reader {
public:
    struct error {
        std::string message;
        std::error_code code;
    };
    
    [[nodiscard]] static std::expected<std::vector<region>, error> 
    read_process_maps(pid_t pid, region_scan_level level = region_scan_level::all) {
        auto maps_path = std::filesystem::path{"/proc"} / std::to_string(pid) / "maps";
        auto exe_path = std::filesystem::path{"/proc"} / std::to_string(pid) / "exe";
        
        if (!std::filesystem::exists(maps_path)) {
            return std::unexpected(error{
                std::format("Maps file {} does not exist", maps_path.string()),
                std::make_error_code(std::errc::no_such_file_or_directory)
            });
        }
        
        std::ifstream maps_file{maps_path};
        if (!maps_file) {
            return std::unexpected(error{
                std::format("Failed to open maps file {}", maps_path.string()),
                std::make_error_code(std::errc::permission_denied)
            });
        }
        
        std::string exe_name;
        if (std::filesystem::exists(exe_path)) {
            try {
                exe_name = std::filesystem::read_symlink(exe_path).string();
            } catch (const std::filesystem::filesystem_error&) {
                // Ignore errors reading exe symlink
            }
        }
        
        std::vector<region> regions;
        std::string line;
        
        unsigned int code_regions = 0;
        unsigned int exe_regions = 0;
        unsigned long prev_end = 0;
        unsigned long load_addr = 0;
        unsigned long exe_load = 0;
        bool is_exe = false;
        std::string bin_name;
        
        while (std::getline(maps_file, line)) {
            if (auto parsed = parse_map_line(line, exe_name, code_regions, 
                                           exe_regions, prev_end, load_addr, 
                                           exe_load, is_exe, bin_name, level)) {
                parsed->id = regions.size();
                regions.push_back(std::move(*parsed));
            }
        }
        
        return regions;
    }

private:
    static std::optional<region> parse_map_line(
        const std::string& line,
        const std::string& exe_name,
        unsigned int& code_regions,
        unsigned int& exe_regions,
        unsigned long& prev_end,
        unsigned long& load_addr,
        unsigned long& exe_load,
        bool& is_exe,
        std::string& bin_name,
        region_scan_level level) {
        
        unsigned long start, end;
        char read, write, exec, cow;
        int offset, dev_major, dev_minor, inode;
        std::string filename;
        
        if (std::sscanf(line.c_str(), "%lx-%lx %c%c%c%c %x %x:%x %d %255s",
                       &start, &end, &read, &write, &exec, &cow, 
                       &offset, &dev_major, &dev_minor, &inode, 
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
        if (code_regions > 0) {
            if (exec == 'x' || 
                (filename != bin_name && (!filename.empty() || start != prev_end)) ||
                code_regions >= 4) {
                code_regions = 0;
                is_exe = false;
                if (exe_regions > 1) {
                    exe_regions = 0;
                }
            } else {
                code_regions++;
                if (is_exe) {
                    exe_regions++;
                }
            }
        }
        
        if (code_regions == 0) {
            if (exec == 'x' && !filename.empty()) {
                code_regions++;
                if (filename == exe_name) {
                    exe_regions = 1;
                    exe_load = start;
                    is_exe = true;
                }
                bin_name = filename;
            } else if (exe_regions == 1 && !filename.empty() && 
                       filename == exe_name) {
                code_regions = ++exe_regions;
                load_addr = exe_load;
                is_exe = true;
                bin_name = filename;
            }
            if (exe_regions < 2) {
                load_addr = start;
            }
        }
        prev_end = end;
        
        // Must have read permissions and non-zero size
        if (read != 'r' || (end - start) == 0) {
            return std::nullopt;
        }
        
        // Determine region type
        region_type type = region_type::misc;
        if (is_exe) {
            type = region_type::exe;
        } else if (code_regions > 0) {
            type = region_type::code;
        } else if (filename == "[heap]") {
            type = region_type::heap;
        } else if (filename == "[stack]") {
            type = region_type::stack;
        }
        
        // Check if region is useful based on scan level
        bool useful = false;
        switch (level) {
            case region_scan_level::all:
                useful = true;
                break;
            case region_scan_level::all_rw:
                useful = true;
                break;
            case region_scan_level::heap_stack_executable_bss:
                if (filename.empty()) {
                    useful = true;
                    break;
                }
                [[fallthrough]];
            case region_scan_level::heap_stack_executable:
                if (type == region_type::heap || type == region_type::stack) {
                    useful = true;
                    break;
                }
                if (type == region_type::exe || filename == exe_name) {
                    useful = true;
                }
                break;
        }
        
        if (!useful) {
            return std::nullopt;
        }
        
        // Skip non-writable regions for non-ALL scan levels
        if (level != region_scan_level::all && write != 'w') {
            return std::nullopt;
        }
        
        region result;
        result.start = reinterpret_cast<void*>(start);
        result.size = end - start;
        result.type = type;
        result.load_addr = reinterpret_cast<void*>(load_addr);
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
[[nodiscard]] inline std::expected<std::vector<region>, maps_reader::error> 
read_process_maps(pid_t pid, region_scan_level level = region_scan_level::all) {
    return maps_reader::read_process_maps(pid, level);
}

