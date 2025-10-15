include(find_boost)

# --- 核心模块库 ---
add_library(NewScanmem_lib STATIC)

# 查找 ICU
find_package(ICU REQUIRED COMPONENTS uc i18n)

target_compile_features(NewScanmem_lib PUBLIC cxx_std_23)

set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-p;build")

# 模块源文件（不带 src/ 前缀）
set(MODULE_SOURCES
    # Utilities
    utils/string.cppm
    utils/parser.cppm
    utils/endianness.cppm
    utils/mem64.cppm
    utils/sets.cppm
    
    # UI abstraction layer
    ui/interface.cppm
    ui/console.cppm
    ui/show_message.cppm
    ui/user_input.cppm
    
    # CLI command system
    cli/command.cppm
    cli/repl.cppm
    cli/commands/help.cppm
    cli/commands/quit.cppm
    cli/commands/pid.cppm
    
    # Core abstraction layer
    core/config.cppm
    core/scanner.cppm
    core/memory.cppm
    core/proc_mem.cppm
    core/targetmem.cppm
    core/maps.cppm
    core/process_checker.cppm
    
    # Core modules
    value/buffer.cppm
    value/view.cppm
    value/cursor.cppm
    value/scalar.cppm
    value/flags.cppm
    value/value.cppm
    value/text.cppm
    scan/types.cppm
    scan/bytes.cppm
    scan/read_helpers.cppm
    scan/string.cppm
    scan/numeric.cppm
    scan/factory.cppm
    scan/engine.cppm
    scanmem.cppm
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
