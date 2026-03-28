#include <CLI/CLI.hpp>

import cli.app;
import cli.app_config;
import utils.version;

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

auto main(int argc, char* argv[]) -> int {
    CLI::App app{"NewScanmem - 现代内存扫描工具\n"
                 "一个使用 C++20 模块重构的 scanmem 现代化版本"};
    app.set_version_flag("--version", std::string("NewScanmem ") + version::string());
    
    cli::AppConfig config;
    
    // PID 选项
    app.add_option("-p,--pid", config.targetPid, 
                   "目标进程 PID")
        ->check(CLI::PositiveNumber);
    
    // 位置参数也接受 PID
    pid_t positionalPid = 0;
    app.add_option("target_pid", positionalPid, "目标进程 PID (位置参数)")
        ->check(CLI::PositiveNumber);
    
    // 初始命令
    app.add_option("-c,--commands", config.initialCommands,
                   "初始执行命令 (分号分隔)")
        ->type_name("cmds");
    
    // 批处理模式
    app.add_flag("-b,--batch", config.batchMode,
                 "批处理模式 (执行命令后退出)");
    
    // 后端模式
    app.add_flag("--backend", config.backendMode,
                 "机器可读模式 (适合脚本解析)");
    
    // 调试模式
    app.add_flag("-d,--debug", config.debugMode,
                 "启用调试输出");
    
    // 禁用颜色
    app.add_flag("--no-color", config.colorMode,
                 "禁用彩色输出 (默认启用)")
        ->default_val(true)
        ->multi_option_policy(CLI::MultiOptionPolicy::TakeLast);
    config.colorMode = true;  // 默认值
    
    // 自定义帮助
    app.set_help_flag("-h,--help", "显示帮助信息并退出");
    
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }
    
    // 处理位置参数 PID (如果提供了)
    if (positionalPid > 0) {
        config.targetPid = positionalPid;
    }
    
    // 批处理模式自动启用后端模式
    if (config.batchMode) {
        config.backendMode = true;
    }
    
    // 颜色模式逻辑：--no-color 会将其设为 false
    if (!config.colorMode) {
        // 已通过命令行禁用
    }
    
    try {
        cli::Application application{config};
        return application.run();
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
}
