module;

#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <expected>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

export module scanmem;

import core.maps;
import ui.show_message;
import core.proc_mem;

// Global message printer instance
static const ui::MessagePrinter G_MSG_PRINTER;

// Convenient namespace-style access
namespace show_message {
template <typename... Args>
inline void info(std::string_view fmt, const Args&... args) {
    G_MSG_PRINTER.info(fmt, args...);
}
template <typename... Args>
inline void warn(std::string_view fmt, const Args&... args) {
    G_MSG_PRINTER.warn(fmt, args...);
}
template <typename... Args>
inline void error(std::string_view fmt, const Args&... args) {
    G_MSG_PRINTER.error(fmt, args...);
}
template <typename... Args>
inline void user(std::string_view fmt, const Args&... args) {
    G_MSG_PRINTER.user(fmt, args...);
}

inline void userPrompt(pid_t pid) {
    if (pid != 0) {
        std::cout << std::format("{}> ", pid) << std::flush;
    } else {
        std::cout << "> " << std::flush;
    }
}
}  // namespace show_message

struct config {
    pid_t targetPid = 0;
    bool debugMode = false;
    bool exitOnError = false;
    std::optional<std::string> initialCommands;

    [[nodiscard]] auto isValid() const noexcept -> bool {
        return targetPid >= 0;
    }
};

class argument_parser {
   public:
    [[nodiscard]] static std::expected<config, std::string> parse(
        int argc, char* argv[]) {
        config result;

        for (int i = 1; i < argc; ++i) {
            std::string_view arg{argv[i]};

            if (arg == "-h" || arg == "--help") {
                return std::unexpected{std::string{get_help_text()}};
            }

            if (arg == "-v" || arg == "--version") {
                return std::unexpected{std::string{get_version_text()}};
            }

            if (arg == "-d" || arg == "--debug") {
                result.debugMode = true;
                continue;
            }

            if (arg == "-e" || arg == "--errexit") {
                result.exitOnError = true;
                continue;
            }

            if (arg.starts_with("-p=") || arg.starts_with("--pid=")) {
                auto pid_str = arg.substr(arg.find('=') + 1);
                if (auto pid = parse_pid(pid_str)) {
                    result.targetPid = *pid;
                } else {
                    return std::unexpected{
                        std::format("Invalid PID: {}", pid_str)};
                }
                continue;
            }

            if (arg.starts_with("-c=") || arg.starts_with("--command=")) {
                result.initialCommands =
                    std::string{arg.substr(arg.find('=') + 1)};
                continue;
            }

            // Handle positional PID argument
            if (arg[0] != '-') {
                if (auto pid = parse_pid(arg)) {
                    result.targetPid = *pid;
                } else {
                    return std::unexpected{std::format("Invalid PID: {}", arg)};
                }
            }
        }

        return result;
    }

   private:
    [[nodiscard]] static std::optional<pid_t> parse_pid(std::string_view str) {
        try {
            auto pid = std::stoul(std::string{str});
            if (pid > 0 && pid <= std::numeric_limits<pid_t>::max()) {
                return static_cast<pid_t>(pid);
            }
        } catch (...) {
            return std::nullopt;
        }
        return std::nullopt;
    }

    [[nodiscard]] static std::string_view get_help_text() noexcept {
        return R"(Usage: scanmem [OPTION]... [PID]
                Interactively locate and modify variables in an executing process.

                -p, --pid=pid		set the target process pid
                -c, --command		run given commands (separated by `;`)
                -h, --help		print this message
                -v, --version		print version information
                -d, --debug		enable debug mode
                -e, --errexit		exit on initial command failure

                scanmem is an interactive debugging utility, enter `help` at the prompt
                for further assistance.
                )";
    }

    [[nodiscard]] static std::string_view get_version_text() noexcept {
        return "NewScanmem version 1.0.0\n";
    }
};

class config_manager {
   public:
    [[nodiscard]] static auto get_config_directory() -> std::filesystem::path {
        if (auto* xdgConfig = std::getenv("XDG_CONFIG_HOME")) {
            return std::filesystem::path{xdgConfig} / "newscanmem";
        }

        if (auto* home = std::getenv("HOME")) {
            return std::filesystem::path{home} / ".config" / "newscanmem";
        }

        if (auto* pwdInfo = getpwuid(getuid())) {
            return std::filesystem::path{pwdInfo->pw_dir} / ".config" /
                   "newscanmem";
        }

        throw std::runtime_error{"Unable to determine config directory"};
    }

    static void ensure_config_directory() {
        auto dir = get_config_directory();
        std::filesystem::create_directories(dir);
    }

    [[nodiscard]] static auto get_history_file() -> std::filesystem::path {
        return get_config_directory() / "history";
    }
};

class command_processor {
   public:
    struct result {
        bool success = true;
        std::optional<std::string> errorMessage;
    };

    [[nodiscard]] static auto executeCommands(const std::string& commands)
        -> std::expected<result, std::string> {
        result res;

        auto command_list = splitCommands(commands);
        for (const auto& cmd : command_list) {
            auto exec_result = executeSingleCommand(cmd);
            if (!exec_result.success) {
                res.success = false;
                res.errorMessage = exec_result.errorMessage;
                return res;
            }
        }

        return res;
    }

    [[nodiscard]] static auto executeSingleCommand(std::string_view /*command*/)
        -> result {
        // Unhandled command placeholder; real commands in interactive_session::execute_command
        // handled there
        return result{.success = true, .errorMessage = std::nullopt};
    }

   private:
    [[nodiscard]] static auto splitCommands(std::string_view commands)
        -> std::vector<std::string> {
        std::vector<std::string> result;
        std::string current;

        for (char chr : commands) {
            if (chr == ';' || chr == '\n') {
                if (!current.empty()) {
                    result.push_back(current);
                    current.clear();
                }
            } else {
                current += chr;
            }
        }

        if (!current.empty()) {
            result.push_back(current);
        }

        return result;
    }
};

class InteractiveSession {
   public:
    struct error {
        std::string message;
        std::error_code code;
    };

    explicit InteractiveSession(config cfg) : cfg_{std::move(cfg)} {}

    [[nodiscard]] std::expected<int, error> run() {
        initialize_session();

        if (cfg_.initialCommands) {
            auto result =
                command_processor::executeCommands(*cfg_.initialCommands);
            if (!result) {
                if (cfg_.exitOnError) {
                    return 1;
                }
                show_message::error("Command execution failed: {}",
                                    result.error());
            }
        }

        if (cfg_.targetPid == 0) {
            show_message::user(
                "Enter the pid of the process to search using the \"pid\" "
                "command.");
            show_message::user("Enter \"help\" for other commands.");
        } else {
            show_message::user(
                "Please enter current value, or \"help\" for other commands.");
        }

        return run_interactive_loop();
    }

   private:
    // --- Minimal command implementations: pid and write ---
    [[nodiscard]] auto executeCommand(std::string_view line) -> bool {
        // Lightweight tokenization (split by whitespace)
        std::vector<std::string> tokens;
        {
            std::string curr;
            for (char chr : line) {
                if (std::isspace(static_cast<unsigned char>(chr)) != 0) {
                    if (!curr.empty()) {
                        tokens.push_back(curr);
                        curr.clear();
                    }
                } else {
                    curr.push_back(chr);
                }
            }
            if (!curr.empty()) {
                tokens.push_back(curr);
            }
        }
        if (tokens.empty()) {
            return true;  // empty command
        }

        auto to_lower = [](std::string str) {
            for (auto& character : str) {
                character = static_cast<char>(
                    std::tolower(static_cast<unsigned char>(character)));
            }
            return str;
        };

        const std::string cmd = to_lower(tokens[0]);

        if (cmd == "pid") {
            if (tokens.size() != 2) {
                show_message::error("usage: pid <pid>");
                return true;
            }
            try {
                auto value = std::stoul(tokens[1]);
                if (value == 0 ||
                    value > static_cast<unsigned long>(
                                std::numeric_limits<pid_t>::max())) {
                    show_message::error("invalid pid: {}", tokens[1]);
                    return true;
                }
                cfg_.targetPid = static_cast<pid_t>(value);
                show_message::info("Target PID set to {}", cfg_.targetPid);
            } catch (...) {
                show_message::error("invalid pid: {}", tokens[1]);
            }
            return true;
        }

        if (cmd == "write") {
            // write <addr> <type> <value>
            if (tokens.size() != 4) {
                show_message::error("usage: write <addr> <type> <value>");
                show_message::user(
                    "type: u8|s8|u16|s16|u32|s32|u64|s64|f32|f64");
                return true;
            }
            if (cfg_.targetPid == 0) {
                show_message::error("set pid first: pid <pid>");
                return true;
            }
            // parse address (hex or dec)
            auto parse_addr =
                [](const std::string& str) -> std::optional<void*> {
                try {
                    unsigned long long val = 0ULL;
                    if (str.size() > 2 &&
                        (str.rfind("0x", 0) == 0 || str.rfind("0X", 0) == 0)) {
                        val = std::stoull(str, nullptr, 16);
                    } else {
                        val = std::stoull(str, nullptr, 10);
                    }
                    return reinterpret_cast<void*>(static_cast<uintptr_t>(val));
                } catch (...) {
                    return std::nullopt;
                }
            };

            auto parse_u64 =
                [](const std::string& str) -> std::optional<uint64_t> {
                try {
                    if (str.size() > 2 &&
                        (str.rfind("0x", 0) == 0 || str.rfind("0X", 0) == 0)) {
                        return std::stoull(str, nullptr, 16);
                    }
                    return std::stoull(str, nullptr, 10);
                } catch (...) {
                    return std::nullopt;
                }
            };
            auto parse_i64 =
                [](const std::string& str) -> std::optional<int64_t> {
                try {
                    if (str.size() > 2 &&
                        (str.rfind("0x", 0) == 0 || str.rfind("0X", 0) == 0)) {
                        return static_cast<int64_t>(
                            std::stoull(str, nullptr, 16));
                    }
                    return static_cast<int64_t>(std::stoll(str, nullptr, 10));
                } catch (...) {
                    return std::nullopt;
                }
            };
            auto parse_f32 =
                [](const std::string& str) -> std::optional<float> {
                try {
                    return std::stof(str);
                } catch (...) {
                    return std::nullopt;
                }
            };
            auto parse_f64 =
                [](const std::string& str) -> std::optional<double> {
                try {
                    return std::stod(str);
                } catch (...) {
                    return std::nullopt;
                }
            };

            auto addrOpt = parse_addr(tokens[1]);
            if (!addrOpt) {
                show_message::error("invalid address: {}", tokens[1]);
                return true;
            }
            const std::string dataType = to_lower(tokens[2]);
            const std::string val = tokens[3];

            auto fail = [&](std::string_view msg) {
                show_message::error("{}", msg);
            };

            // dispatch by type
            if (dataType == "u8") {
                if (auto value = parse_u64(val)) {
                    auto res = writeValue<uint8_t>(
                        cfg_.targetPid, *addrOpt, static_cast<uint8_t>(*value));
                    if (!res) {
                        fail(res.error());
                    } else {
                        show_message::info("wrote {} byte(s)", *res);
                    }
                } else {
                    fail("invalid value");
                }
            } else if (dataType == "s8") {
                if (auto value = parse_i64(val)) {
                    auto res = writeValue<int8_t>(cfg_.targetPid, *addrOpt,
                                                  static_cast<int8_t>(*value));
                    if (!res) {
                        fail(res.error());
                    } else {
                        show_message::info("wrote {} byte(s)", *res);
                    }
                } else {
                    fail("invalid value");
                }
            } else if (dataType == "u16") {
                if (auto value = parse_u64(val)) {
                    auto res =
                        writeValue<uint16_t>(cfg_.targetPid, *addrOpt,
                                             static_cast<uint16_t>(*value));
                    if (!res) {
                        fail(res.error());
                    } else {
                        show_message::info("wrote {} byte(s)", *res);
                    }
                } else {
                    fail("invalid value");
                }
            } else if (dataType == "s16") {
                if (auto value = parse_i64(val)) {
                    auto res = writeValue<int16_t>(
                        cfg_.targetPid, *addrOpt, static_cast<int16_t>(*value));
                    if (!res) {
                        fail(res.error());
                    } else {
                        show_message::info("wrote {} byte(s)", *res);
                    }
                } else {
                    fail("invalid value");
                }
            } else if (dataType == "u32") {
                if (auto value = parse_u64(val)) {
                    auto res =
                        writeValue<uint32_t>(cfg_.targetPid, *addrOpt,
                                             static_cast<uint32_t>(*value));
                    if (!res) {
                        fail(res.error());
                    } else {
                        show_message::info("wrote {} byte(s)", *res);
                    }
                } else {
                    fail("invalid value");
                }
            } else if (dataType == "s32") {
                if (auto value = parse_i64(val)) {
                    auto res = writeValue<int32_t>(
                        cfg_.targetPid, *addrOpt, static_cast<int32_t>(*value));
                    if (!res) {
                        fail(res.error());
                    } else {
                        show_message::info("wrote {} byte(s)", *res);
                    }
                } else {
                    fail("invalid value");
                }
            } else if (dataType == "u64") {
                if (auto value = parse_u64(val)) {
                    auto res =
                        writeValue<uint64_t>(cfg_.targetPid, *addrOpt,
                                             static_cast<uint64_t>(*value));
                    if (!res) {
                        fail(res.error());
                    } else {
                        show_message::info("wrote {} byte(s)", *res);
                    }
                } else {
                    fail("invalid value");
                }
            } else if (dataType == "s64") {
                if (auto value = parse_i64(val)) {
                    auto res = writeValue<int64_t>(
                        cfg_.targetPid, *addrOpt, static_cast<int64_t>(*value));
                    if (!res) {
                        fail(res.error());
                    } else {
                        show_message::info("wrote {} byte(s)", *res);
                    }
                } else {
                    fail("invalid value");
                }
            } else if (dataType == "f32") {
                if (auto value = parse_f32(val)) {
                    auto res =
                        writeValue<float>(cfg_.targetPid, *addrOpt, *value);
                    if (!res) {
                        fail(res.error());
                    } else {
                        show_message::info("wrote {} byte(s)", *res);
                    }
                } else {
                    fail("invalid value");
                }
            } else if (dataType == "f64") {
                if (auto value = parse_f64(val)) {
                    auto res =
                        writeValue<double>(cfg_.targetPid, *addrOpt, *value);
                    if (!res) {
                        fail(res.error());
                    } else {
                        show_message::info("wrote {} byte(s)", *res);
                    }
                } else {
                    fail("invalid value");
                }
            } else {
                show_message::error("unknown type: {}", tokens[2]);
            }
            return true;
        }

        return false;  // unhandled
    }
    void initialize_session() {
        try {
            config_manager::ensure_config_directory();
            load_history();

            if (cfg_.debugMode) {
                show_message::info("Debug mode enabled");
            }

            if (cfg_.targetPid != 0) {
                show_message::info("Target PID: {}", cfg_.targetPid);
            }

        } catch (const std::exception& e) {
            show_message::error("Session initialization failed: {}", e.what());
        }
    }

    void load_history() {
        // Placeholder for history loading
        // Would integrate with readline or similar
    }

    [[nodiscard]] std::expected<int, error> run_interactive_loop() {
        std::string input_line;

        while (true) {
            show_message::userPrompt(cfg_.targetPid);

            if (!std::getline(std::cin, input_line)) {
                break;
            }

            if (input_line.empty()) {
                continue;
            }

            if (input_line == "quit" || input_line == "exit") {
                break;
            }

            if (input_line == "help") {
                show_help();
                continue;
            }

            // First try built-ins (pid, write); delegate unhandled to placeholder executor
            if (executeCommand(input_line)) {
                continue;
            }
            auto result = command_processor::executeSingleCommand(input_line);
            if (!result.success) {
                show_message::error(
                    "Command failed: {}",
                    result.errorMessage.value_or("Unknown error"));
            }
        }

        save_history();
        return 0;
    }

    void save_history() {
        // Placeholder for history saving
    }

    void show_help() {
        show_message::user(R"(
Available commands:
  pid <pid>     - Set target process PID
  write <addr> <type> <value> - Write value to target memory
  reset        - Reset the search
  help         - Show this help
  quit/exit    - Exit the program
  version      - Show version information
)");
    }

    config cfg_;
};

// Main entry point for the scanmem functionality
[[nodiscard]] std::expected<int, std::string> run_scanmem(const config& cfg) {
    if (getuid() != 0) {
        show_message::warn("Run as root if memory regions are missing.");
    }

    try {
        InteractiveSession session{cfg};
        auto result = session.run();

        if (result) {
            return *result;
        }
        return std::unexpected{
            std::format("Session error: {}", result.error().message)};

    } catch (const std::exception& e) {
        return std::unexpected{std::format("Runtime error: {}", e.what())};
    }
}
