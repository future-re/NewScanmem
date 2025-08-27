# 测试子目录的 CMakeLists.txt

# 启用测试
enable_testing()

# 查找 GTest（假设已在根 CMakeLists.txt 中配置）
find_package(GTest REQUIRED)

set(TEST_LIBS GTest::GTest GTest::Main NewScanmem_lib)

# Sets 模块测试
add_executable(test_sets test_sets.cpp)
target_link_libraries(test_sets PRIVATE ${TEST_LIBS})

# ProcessChecker 模块测试
add_executable(test_process_checker test_process_checker.cpp)
target_link_libraries(test_process_checker PRIVATE ${TEST_LIBS})

# Endianness 模块测试
add_executable(test_endianness test_endianness.cpp)
target_link_libraries(test_endianness PRIVATE ${TEST_LIBS})

# 发现并运行测试
include(GoogleTest)
gtest_discover_tests(test_sets)
gtest_discover_tests(test_process_checker)
gtest_discover_tests(test_endianness)