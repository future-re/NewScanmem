# 测试子目录的 CMakeLists.txt

# 启用测试（通常在顶层调用一次，这里保留注释说明）
# enable_testing()

# 查找 GTest 并引入 GoogleTest CMake 辅助
find_package(GTest REQUIRED)
include(GoogleTest)

# 统一的测试依赖库集合
set(TEST_LIBS GTest::GTest GTest::Main NewScanmem_lib)

# 自动收集当前目录下的单元测试源文件，排除单独管理的集成测试与 e2e 测试
file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

list(FILTER TEST_SOURCES EXCLUDE REGEX ".*/integration/.*")
list(FILTER TEST_SOURCES EXCLUDE REGEX ".*/e2e/.*")

# 为每个测试源生成可执行文件并注册 gtest_discover_tests
foreach(TEST_SRC ${TEST_SOURCES})
    file(RELATIVE_PATH TEST_REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}" "${TEST_SRC}")
    get_filename_component(TEST_NAME "${TEST_SRC}" NAME_WE)
    string(REPLACE "/" "_" TEST_TARGET_SUFFIX "${TEST_REL_PATH}")
    string(REPLACE ".cpp" "" TEST_TARGET_SUFFIX "${TEST_TARGET_SUFFIX}")
    set(TEST_TARGET "test_${TEST_TARGET_SUFFIX}")

    add_executable(${TEST_TARGET} ${TEST_SRC})
    target_link_libraries(${TEST_TARGET} PRIVATE ${TEST_LIBS})

    # 自动发现测试点
    gtest_discover_tests(${TEST_TARGET}
        DISCOVERY_MODE PRE_TEST
        PROPERTIES
            DISCOVERED_TEST_PREFIX "${TEST_NAME}::"
            LABELS "unit"
    )
endforeach()

# 集成测试子目录（与单元测试分离构建与运行）
add_subdirectory(integration)
