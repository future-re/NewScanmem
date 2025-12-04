# 集成测试 CMake 配置

# ========================================
# 1. 编译测试目标程序（被扫描的进程）
# ========================================
add_executable(target_fixed_int targets/target_fixed_int.cpp)
add_executable(target_fixed_double targets/target_fixed_double.cpp)

# ========================================
# 2. 注册集成测试
# ========================================

# 基础测试：CLI 帮助信息
add_test(
    NAME Integration_CLI_Help
    COMMAND $<TARGET_FILE:NewScanmem> --help
)
set_tests_properties(Integration_CLI_Help PROPERTIES LABELS "integration")

# 集成测试：扫描和修改 int64 类型值
add_test(
    NAME Integration_Scan_Int64
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_integration.py 
            target_fixed_int int64 1122334455667788
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_tests_properties(Integration_Scan_Int64 PROPERTIES 
    LABELS "integration"
    TIMEOUT 60
)

# 集成测试：扫描和修改 double 类型值
add_test(
    NAME Integration_Scan_Double
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_integration.py 
            target_fixed_double float64 12345.6789
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_tests_properties(Integration_Scan_Double PROPERTIES 
    LABELS "integration"
    TIMEOUT 60
)
