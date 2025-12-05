# 集成测试目标程序编译配置
# 这些是用于测试 NewScanmem 的目标进程
# 使用方式：手动运行 test_integration.py 脚本进行测试

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

# 说明：
# 运行集成测试请使用以下命令（在 build 目录下）：
# python3 ../test/integration/test_integration.py target_fixed_int int64 1122334455667788 9999999999999999

