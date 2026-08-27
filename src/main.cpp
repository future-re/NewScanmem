#include <charconv>
#include <iostream>
#include <string>
#include <string_view>

#include "newscanmem/cli/app.hpp"
#include "newscanmem/cli/app_config.hpp"
#include "newscanmem/utils/version.hpp"

namespace {
auto parsePid(std::string_view text, pid_t& output) -> bool {
    if (text.empty()) return false;
    auto [end, ec] =
        std::from_chars(text.data(), text.data() + text.size(), output);
    return ec == std::errc{} && end == text.data() + text.size() && output > 0;
}

auto printUsage(std::string_view program) -> void {
    std::cout << "Usage: " << program << " [options] [pid]\n"
              << "  -p, --pid PID       target process\n"
              << "  -c, --commands CMD  semicolon-separated commands\n"
              << "  -b, --batch         execute commands and exit\n"
              << "      --backend      machine-readable mode\n"
              << "  -d, --debug         enable debug output\n"
              << "      --no-color      disable colors\n"
              << "      --version       print version\n"
              << "  -h, --help          show this help\n";
}

auto parseArgs(int argc, char* argv[], cli::AppConfig& config) -> bool {
    for (int index = 1; index < argc; ++index) {
        const std::string_view arg = argv[index];
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return false;
        }
        if (arg == "--version") {
            std::cout << "NewScanmem " << version::string() << '\n';
            return false;
        }
        if (arg == "-b" || arg == "--batch") {
            config.batchMode = true;
            config.backendMode = true;
            continue;
        }
        if (arg == "-d" || arg == "--debug") {
            config.debugMode = true;
            continue;
        }
        if (arg == "--backend") {
            config.backendMode = true;
            continue;
        }
        if (arg == "--no-color") {
            config.colorMode = false;
            continue;
        }

        std::string_view value;
        if (arg == "-p" || arg == "--pid" || arg == "-c" ||
            arg == "--commands") {
            if (++index >= argc) {
                std::cerr << "Missing value for " << arg << '\n';
                return false;
            }
            value = argv[index];
            if (arg == "-c" || arg == "--commands") {
                config.initialCommands = std::string(value);
                continue;
            }
        } else if (arg.starts_with("--pid=")) {
            value = arg.substr(6);
        } else {
            pid_t positional = 0;
            if (!parsePid(arg, positional)) {
                std::cerr << "Unknown option or invalid PID: " << arg << '\n';
                return false;
            }
            config.targetPid = positional;
            continue;
        }

        pid_t pid = 0;
        if (!parsePid(value, pid)) {
            std::cerr << "Invalid PID: " << value << '\n';
            return false;
        }
        config.targetPid = pid;
    }
    return true;
}
}  // namespace

auto main(int argc, char* argv[]) -> int {
    cli::AppConfig config;
    if (!parseArgs(argc, argv, config)) return 0;
    try {
        cli::Application application{config};
        return application.run();
    } catch (const std::exception& error) {
        std::cerr << "错误: " << error.what() << '\n';
        return 1;
    }
}
