# 集成测试 CMake 配置

# 可选：为集成测试目标设置独立输出目录
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/integration)

# 目标程序：提供可扫描的进程（打印 PID 和固定内存值）
add_executable(target_fixed_int targets/target_fixed_int.cpp)

# 使用 CTest 注册集成测试（不依赖 gtest）
# 1) CLI 基础可用性（--help）
add_test(
    NAME Integration_CLI_Help
    COMMAND $<TARGET_FILE:NewScanmem> --help
)
set_tests_properties(Integration_CLI_Help PROPERTIES LABELS "integration")

# 2) 目标进程能正常运行一段时间（1秒快速测试）
add_test(
    NAME Integration_Target_Runs
    COMMAND $<TARGET_FILE:target_fixed_int> --duration-ms 1000
)
set_tests_properties(Integration_Target_Runs PROPERTIES LABELS "integration" TIMEOUT 5)
