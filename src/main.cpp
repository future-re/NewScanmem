import cli.app;
import cli.app_config;
import utils.version;

#include <cstdlib>
#include <iostream>

auto main(int argc, char* argv[]) -> int {
    cli::AppConfig config;

    // 参数解析：支持位置参数 <pid> 以及 -p/--pid 形式
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--version") {
            std::cout << "NewScanmem " << version::string() << "\n";
            return 0;
        }
        if ((arg == "-p" || arg == "--pid") && i + 1 < argc) {
            config.targetPid = std::atoi(argv[++i]);
        } else if (!arg.empty() && arg[0] != '-') {
            // 位置参数：第一个非选项参数若为数字，则认为是 pid
            bool numeric = true;
            for (char chDigit : arg) {
                if (chDigit < '0' || chDigit > '9') {
                    numeric = false;
                    break;
                }
            }
            if (numeric) {
                config.targetPid = std::atoi(arg.c_str());
                // 吞掉后续位置参数（若有）但继续解析其他选项
                continue;
            }
        } else if (arg == "-d" || arg == "--debug") {
            config.debugMode = true;
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "用法: scanmem [选项]\n"
                      << "选项:\n"
                      << "  -p, --pid <pid>   目标进程 PID\n"
                      << "  -d, --debug       调试模式\n"
                      << "  --version         显示版本\n"
                      << "  -h, --help        显示帮助\n";
            return 0;
        }
    }

    cli::Application app{config};
    return app.run();
}
