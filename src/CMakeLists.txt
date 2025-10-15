include(find_boost)

# --- 核心模块库 ---
add_library(NewScanmem_lib STATIC)

# 查找 ICU
find_package(ICU REQUIRED COMPONENTS uc i18n)

target_compile_features(NewScanmem_lib PUBLIC cxx_std_23)

set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-p;build")

# 模块源文件（不带 src/ 前缀）
set(MODULE_SOURCES
    show_message.cppm
    endianness.cppm
    common.cppm
    value/buffer.cppm
    value/view.cppm
    value/cursor.cppm
    value/scalar.cppm
    value/flags.cppm
    value/value.cppm
    value/text.cppm
    mem64.cppm
    user_input.cppm
    targetmem.cppm
    sets.cppm
    process_checker.cppm
    maps.cppm
    scan/types.cppm
    scan/spec.cppm
    scan/bytes.cppm
    scan/string.cppm
    scan/numeric.cppm
    scan/factory.cppm
)

# 添加模块源文件到库
target_sources(NewScanmem_lib
    PUBLIC
        FILE_SET CXX_MODULES FILES
        ${MODULE_SOURCES}
)

target_link_libraries(NewScanmem_lib PUBLIC Boost::filesystem Boost::regex ICU::uc ICU::i18n)

# --- 主可执行文件 ---
add_executable(NewScanmem main.cpp)
target_link_libraries(NewScanmem PRIVATE NewScanmem_lib)
