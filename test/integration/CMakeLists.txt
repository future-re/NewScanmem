# 集成测试 CMake 配置

# ========================================
# 1. 编译测试目标程序（被扫描的进程）
# ========================================
set(INTEGRATION_TARGETS
    target_fixed_int
    target_fixed_double
    target_array_element
    target_struct_field
    target_multiple_values
)

foreach(target ${INTEGRATION_TARGETS})
    add_executable(${target} targets/${target}.cpp)
endforeach()

# ========================================
# 2. 注册集成测试
# ========================================

# 基础测试：CLI 帮助信息
add_test(
    NAME Integration_CLI_Help
    COMMAND $<TARGET_FILE:NewScanmem> --help
)
set_tests_properties(Integration_CLI_Help PROPERTIES LABELS "integration")

# 辅助函数：添加集成测试
function(add_integration_test test_name target_name value_type expected_value modified_value)
    add_test(
        NAME ${test_name}
        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/test_integration.py 
                ${target_name} ${value_type} ${expected_value} ${modified_value}
                --wait-modify-ms 10000
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    set_tests_properties(${test_name} PROPERTIES 
        LABELS "integration"
        TIMEOUT 30
    )
endfunction()

# 注册各项集成测试
add_integration_test(Integration_Scan_Int64        target_fixed_int       int64   1122334455667788  9999999999999999)
add_integration_test(Integration_Scan_Multiple   target_multiple_values  FLOAT_32 3.14159          9.87654)
add_integration_test(Integration_Scan_Double       target_fixed_double    FLOAT_64 12345.6789        99999.9999)
add_integration_test(Integration_Scan_Array_Element target_array_element  int32   500               9999)
add_integration_test(Integration_Scan_Struct_Field target_struct_field    int32   9999              1000000)

