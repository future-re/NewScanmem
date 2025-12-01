import cli.app;
import cli.app_config;

#include <cstdlib>
#include <iostream>

auto main(int argc, char* argv[]) -> int {
    cli::AppConfig config;

    // 简化参数解析：只处理 -p <pid>
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-p" || arg == "--pid") && i + 1 < argc) {
            config.targetPid = std::atoi(argv[++i]);
        } else if (arg == "-d" || arg == "--debug") {
            config.debugMode = true;
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "用法: scanmem [选项]\n"
                      << "选项:\n"
                      << "  -p, --pid <pid>   目标进程 PID\n"
                      << "  -d, --debug       调试模式\n"
                      << "  -h, --help        显示帮助\n";
            return 0;
        }
    }

    cli::Application app{config};
    return app.run();
}
