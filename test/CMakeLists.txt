# 测试子目录的 CMakeLists.txt

# 启用测试（通常在顶层调用一次，这里保留注释说明）
# enable_testing()

# 查找 GTest 并引入 GoogleTest CMake 辅助
find_package(GTest REQUIRED)
include(GoogleTest)

# 统一的测试依赖库集合
set(TEST_LIBS GTest::GTest GTest::Main NewScanmem_lib)

# 测试源文件列表
set(TEST_SOURCES
    utils/test_sets.cpp
    utils/test_endianness.cpp
    value/test_scalar.cpp
    core/test_maps.cpp
    core/test_memory.cpp
)

# 为每个测试源生成可执行文件并注册 gtest_discover_tests
foreach(TEST_SRC ${TEST_SOURCES})
    get_filename_component(TEST_NAME "${TEST_SRC}" NAME_WE)
    set(TEST_TARGET "test_${TEST_NAME}") # 添加前缀以避免目标名冲突

    add_executable(${TEST_TARGET} ${TEST_SRC})
    target_link_libraries(${TEST_TARGET} PRIVATE ${TEST_LIBS})

    # 自动发现测试点
    gtest_discover_tests(${TEST_TARGET}
        DISCOVERY_MODE PRE_TEST
        PROPERTIES DISCOVERED_TEST_PREFIX "${TEST_NAME}::"
    )
endforeach()