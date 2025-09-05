include(find_boost)

# --- 核心模块库 ---
add_library(NewScanmem_lib STATIC)

target_compile_features(NewScanmem_lib PUBLIC cxx_std_23)

set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-p;build")

# 模块源文件（不带 src/ 前缀）
set(MODULE_SOURCES
    show_message.cppm
    value.cppm
    targetmem.cppm
    sets.cppm
    process_checker.cppm
    endianness.cppm
    maps.cppm
    common.cppm
    scanroutines.cppm
)

# 添加模块源文件到库
target_sources(NewScanmem_lib
    PUBLIC
        FILE_SET CXX_MODULES FILES
        ${MODULE_SOURCES}
)

target_link_libraries(NewScanmem_lib PUBLIC Boost::filesystem Boost::regex)

# --- 主可执行文件 ---
add_executable(NewScanmem main.cpp)
target_link_libraries(NewScanmem PRIVATE NewScanmem_lib)