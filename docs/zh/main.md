# 主应用文档

## 概述

`main.cpp` 文件作为 NewScanmem 应用程序的入口点。它演示了基本的模块集成，并为内存扫描工具提供了基础框架。

## 文件结构

```cpp
import sets;

int main() {
    Set val;
    return 0;
}
```

## 当前实现

当前的主函数是最小化的，作为以下功能的占位符：

1. **模块集成测试**: 演示 `sets` 模块可以成功导入和使用
2. **基本框架**: 为应用程序开发提供起点
3. **编译验证**: 确保所有模块能够正确编译在一起

## 计划功能

### 命令行界面

```cpp
// 未来实现
int main(int argc, char* argv[]) {
    // 解析命令行参数
    // 初始化模块
    // 开始内存扫描
    // 显示结果
}
```

### 集成点

主应用程序将集成所有模块：

1. **进程管理** (process_checker)
   - 目标进程选择
   - 进程状态监控
   - 权限检查

2. **内存分析** (targetmem)
   - 内存区域扫描
   - 值匹配
   - 模式检测

3. **数据类型** (value)
   - 多类型值支持
   - 字节序处理
   - 字节数组操作

4. **实用工具** (sets, endianness, show_message)
   - 结果集合操作
   - 字节序转换
   - 用户消息和日志

## 使用示例

### 基本执行

```bash
# 当前用法
./newscanmem

# 未来用法示例
./newscanmem --pid 1234 --type int32 --value 42
./newscanmem --pid 1234 --range 0x1000-0x2000 --string "hello"
./newscanmem --pid 1234 --float --tolerance 0.001
```

## 开发路线图

### 第一阶段：基本框架

- [ ] 命令行参数解析
- [ ] 进程选择和验证
- [ ] 基本内存扫描
- [ ] 结果显示

### 第二阶段：高级功能

- [ ] 多值类型支持
- [ ] 内存区域过滤
- [ ] 模式匹配
- [ ] 交互模式

### 第三阶段：优化

- [ ] 性能调优
- [ ] 内存使用优化
- [ ] 并行处理
- [ ] 结果缓存

### 第四阶段：用户界面/用户体验

- [ ] 进度指示器
- [ ] 彩色输出
- [ ] 交互式界面
- [ ] 配置文件支持

## 模块集成

### 进程检查器集成

```cpp
import process_checker;

// 检查目标进程状态
pid_t targetPid = 1234;
ProcessState state = ProcessChecker::check_process(targetPid);

if (state == ProcessState::RUNNING) {
    std::cout << "进程正在运行" << std::endl;
} else {
    std::cout << "进程未运行或无法访问" << std::endl;
    return 1;
}
```

### 内存扫描集成

```cpp
import targetmem;

// 创建内存匹配数组
MatchesAndOldValuesArray matches;

// 执行内存扫描
// ... 扫描逻辑 ...

// 显示结果
for (const auto& swath : matches.swaths) {
    std::cout << "找到匹配的内存区域" << std::endl;
}
```

### 值类型处理

```cpp
import value;

// 创建搜索值
Value searchValue;
searchValue.value = static_cast<int32_t>(42);
searchValue.flags = MatchFlags::S32B;

// 执行值匹配
// ... 匹配逻辑 ...
```

### 消息系统集成

```cpp
import show_message;

// 创建消息打印机
MessageContext ctx;
ctx.debugMode = true;
MessagePrinter printer(ctx);

printer.info("开始内存扫描");
printer.debug("调试信息: 扫描参数");
printer.warn("警告: 内存使用率较高");
```

## 命令行参数设计

### 基本参数

```cpp
struct CommandLineArgs {
    pid_t targetPid = 0;           // 目标进程 ID
    std::string valueType = "int32"; // 值类型
    std::string searchValue;       // 搜索值
    std::string memoryRange;       // 内存范围
    bool debugMode = false;        // 调试模式
    bool verbose = false;          // 详细输出
};
```

### 参数解析

```cpp
// 解析命令行参数
CommandLineArgs parseArgs(int argc, char* argv[]) {
    CommandLineArgs args;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--pid" && i + 1 < argc) {
            args.targetPid = std::stoi(argv[++i]);
        } else if (arg == "--type" && i + 1 < argc) {
            args.valueType = argv[++i];
        } else if (arg == "--value" && i + 1 < argc) {
            args.searchValue = argv[++i];
        } else if (arg == "--range" && i + 1 < argc) {
            args.memoryRange = argv[++i];
        } else if (arg == "--debug") {
            args.debugMode = true;
        } else if (arg == "--verbose") {
            args.verbose = true;
        }
    }
    
    return args;
}
```

## 错误处理

### 参数验证

```cpp
bool validateArgs(const CommandLineArgs& args) {
    if (args.targetPid <= 0) {
        std::cerr << "错误: 无效的进程 ID" << std::endl;
        return false;
    }
    
    if (args.searchValue.empty()) {
        std::cerr << "错误: 未指定搜索值" << std::endl;
        return false;
    }
    
    return true;
}
```

### 进程验证

```cpp
bool validateProcess(pid_t pid) {
    ProcessState state = ProcessChecker::check_process(pid);
    
    if (state == ProcessState::DEAD) {
        std::cerr << "错误: 进程不存在" << std::endl;
        return false;
    }
    
    if (state == ProcessState::ERROR) {
        std::cerr << "错误: 无法访问进程" << std::endl;
        return false;
    }
    
    return true;
}
```

## 性能考虑

### 内存使用

1. **流式处理**: 避免一次性加载大量数据
2. **内存映射**: 使用内存映射文件处理大文件
3. **缓存策略**: 实现智能缓存减少重复计算

### 并行处理

```cpp
// 并行扫描多个内存区域
void parallelScan(const std::vector<MemoryRegion>& regions) {
    std::vector<std::thread> threads;
    
    for (const auto& region : regions) {
        threads.emplace_back([region]() {
            scanRegion(region);
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}
```

### 进度报告

```cpp
class ProgressReporter {
private:
    size_t m_total;
    size_t m_current;
    std::chrono::steady_clock::time_point m_start;
    
public:
    ProgressReporter(size_t total) : m_total(total), m_current(0) {
        m_start = std::chrono::steady_clock::now();
    }
    
    void update(size_t increment = 1) {
        m_current += increment;
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_start);
        
        float progress = static_cast<float>(m_current) / m_total * 100.0f;
        std::cout << "\r进度: " << std::fixed << std::setprecision(1) 
                  << progress << "% (" << m_current << "/" << m_total << ")" 
                  << std::flush;
    }
};
```

## 配置管理

### 配置文件支持

```cpp
struct Config {
    bool debugMode = false;
    bool verbose = false;
    size_t maxResults = 1000;
    std::string logFile;
    std::map<std::string, std::string> customSettings;
};

Config loadConfig(const std::string& configFile) {
    Config config;
    // 从文件加载配置
    return config;
}
```

### 环境变量

```cpp
void loadEnvironmentConfig() {
    if (const char* debug = std::getenv("NEWSCANMEM_DEBUG")) {
        if (std::string(debug) == "1") {
            // 启用调试模式
        }
    }
}
```

## 测试和调试

### 单元测试

```cpp
// 测试模块集成
void testModuleIntegration() {
    // 测试 sets 模块
    Set testSet;
    assert(testSet.size() == 0);
    
    // 测试 value 模块
    Value testValue;
    testValue.value = static_cast<int32_t>(42);
    assert(std::holds_alternative<int32_t>(testValue.value));
}
```

### 调试工具

```cpp
class DebugHelper {
public:
    static void dumpMemory(const void* addr, size_t size) {
        const uint8_t* bytes = static_cast<const uint8_t*>(addr);
        
        for (size_t i = 0; i < size; ++i) {
            if (i % 16 == 0) {
                std::cout << std::endl << std::hex << std::setw(8) 
                          << std::setfill('0') << i << ": ";
            }
            std::cout << std::hex << std::setw(2) << std::setfill('0') 
                      << static_cast<int>(bytes[i]) << " ";
        }
        std::cout << std::endl;
    }
};
```

## 未来扩展

### 插件系统

```cpp
// 插件接口
class ScanPlugin {
public:
    virtual bool initialize() = 0;
    virtual bool scan(const MemoryRegion& region) = 0;
    virtual void cleanup() = 0;
    virtual std::string getName() const = 0;
};

// 插件管理器
class PluginManager {
private:
    std::vector<std::unique_ptr<ScanPlugin>> m_plugins;
    
public:
    void registerPlugin(std::unique_ptr<ScanPlugin> plugin);
    void runPlugins(const MemoryRegion& region);
};
```

### 网络支持

```cpp
// 远程扫描支持
class RemoteScanner {
public:
    bool connect(const std::string& host, int port);
    bool scanRemoteProcess(pid_t pid);
    std::vector<MemoryMatch> getResults();
};
```

### GUI 界面

```cpp
// 图形用户界面（未来计划）
class MainWindow {
public:
    void showProcessList();
    void showScanResults();
    void showMemoryMap();
};
```
