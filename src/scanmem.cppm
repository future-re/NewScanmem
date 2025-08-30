module;

#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <array>
#include <concepts>
#include <expected>
#include <filesystem>
#include <format>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>

export module scanmem;

import maps;
import show_message;

struct config {
    pid_t target_pid = 0;
    bool debug_mode = false;
    bool exit_on_error = false;
    std::optional<std::string> initial_commands;

    [[nodiscard]] bool is_valid() const noexcept { return target_pid >= 0; }
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
                result.debug_mode = true;
                continue;
            }

            if (arg == "-e" || arg == "--errexit") {
                result.exit_on_error = true;
                continue;
            }

            if (arg.starts_with("-p=") || arg.starts_with("--pid=")) {
                auto pid_str = arg.substr(arg.find('=') + 1);
                if (auto pid = parse_pid(pid_str)) {
                    result.target_pid = *pid;
                } else {
                    return std::unexpected{
                        std::format("Invalid PID: {}", pid_str)};
                }
                continue;
            }

            if (arg.starts_with("-c=") || arg.starts_with("--command=")) {
                result.initial_commands =
                    std::string{arg.substr(arg.find('=') + 1)};
                continue;
            }

            // Handle positional PID argument
            if (arg[0] != '-') {
                if (auto pid = parse_pid(arg)) {
                    result.target_pid = *pid;
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
    [[nodiscard]] static std::filesystem::path get_config_directory() {
        if (auto xdg_config = std::getenv("XDG_CONFIG_HOME")) {
            return std::filesystem::path{xdg_config} / "newscanmem";
        }

        if (auto home = std::getenv("HOME")) {
            return std::filesystem::path{home} / ".config" / "newscanmem";
        }

        if (auto pw = getpwuid(getuid())) {
            return std::filesystem::path{pw->pw_dir} / ".config" / "newscanmem";
        }

        throw std::runtime_error{"Unable to determine config directory"};
    }

    static void ensure_config_directory() {
        auto dir = get_config_directory();
        std::filesystem::create_directories(dir);
    }

    [[nodiscard]] static std::filesystem::path get_history_file() {
        return get_config_directory() / "history";
    }
};

class command_processor {
   public:
    struct result {
        bool success = true;
        std::optional<std::string> error_message;
    };

    [[nodiscard]] static std::expected<result, std::string> execute_commands(
        const std::string& commands) {
        result res;

        auto command_list = split_commands(commands);
        for (const auto& cmd : command_list) {
            auto exec_result = execute_single_command(cmd);
            if (!exec_result.success) {
                res.success = false;
                res.error_message = exec_result.error_message;
                return res;
            }
        }

        return res;
    }

   private:
    [[nodiscard]] static std::vector<std::string> split_commands(
        std::string_view commands) {
        std::vector<std::string> result;
        std::string current;

        for (char c : commands) {
            if (c == ';' || c == '\n') {
                if (!current.empty()) {
                    result.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }

        if (!current.empty()) {
            result.push_back(current);
        }

        return result;
    }

    [[nodiscard]] static result execute_single_command(
        std::string_view command) {
        // Placeholder for actual command execution
        // In a real implementation, this would integrate with the actual
        // scanmem core
        return result{true, std::nullopt};
    }
};

class interactive_session {
   public:
    struct error {
        std::string message;
        std::error_code code;
    };

    explicit interactive_session(const config& cfg) : cfg_{cfg} {}

    [[nodiscard]] std::expected<int, error> run() {
        initialize_session();

        if (cfg_.initial_commands) {
            auto result =
                command_processor::execute_commands(*cfg_.initial_commands);
            if (!result) {
                if (cfg_.exit_on_error) {
                    return 1;
                }
                show_message::error("Command execution failed: {}",
                                    result.error());
            }
        }

        if (cfg_.target_pid == 0) {
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
    void initialize_session() {
        try {
            config_manager::ensure_config_directory();
            load_history();

            if (cfg_.debug_mode) {
                show_message::info("Debug mode enabled");
            }

            if (cfg_.target_pid != 0) {
                show_message::info("Target PID: {}", cfg_.target_pid);
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
            show_message::user_prompt(cfg_.target_pid);

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

            auto result = command_processor::execute_single_command(input_line);
            if (!result.success) {
                show_message::error(
                    "Command failed: {}",
                    result.error_message.value_or("Unknown error"));
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
        interactive_session session{cfg};
        auto result = session.run();

        if (result) {
            return *result;
        } else {
            return std::unexpected{
                std::format("Session error: {}", result.error().message)};
        }
    } catch (const std::exception& e) {
        return std::unexpected{std::format("Runtime error: {}", e.what())};
    }
}
