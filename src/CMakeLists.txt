# --- Core header library ---
# The project previously exposed every implementation unit as a C++20 module.
# Keep the public surface as ordinary headers so GCC/Clang/CMake versions do
# not need BMI/module-scanning support.
add_library(NewScanmem_lib STATIC
  app/result_service.cpp
  app/scan_service.cpp
  core/memory.cpp
  core/memory_writer.cpp
  core/match_formatter.cpp
  core/match.cpp
  core/maps.cpp
  core/process_checker.cpp
  core/proc_mem.cpp
  core/region_classifier.cpp
  core/region_filter.cpp
  core/scan_history.cpp
  core/scanner.cpp
  scan/job.cpp
  scan/factory.cpp
  scan/routine.cpp
  scan/engine.cpp
  scan/filter.cpp
  scan/match_storage.cpp
  scan/numeric.cpp
  scan/bytes.cpp
  scan/string.cpp
  value/flags.cpp
  value/core.cpp
  value/parser.cpp
  ui/show_message.cpp
  ui/terminal.cpp
  ui/line_editor.cpp
  ui/console.cpp
  ui/interface.cpp
  ui/user_input.cpp
  cli/command.cpp
  cli/session.cpp
  cli/repl.cpp
  cli/app_config.cpp
  cli/app.cpp
    cli/commands/count.cpp
    cli/commands/help.cpp
    cli/commands/list.cpp
  cli/commands/reset.cpp
  cli/commands/quit.cpp
  cli/commands/pid.cpp
    cli/commands/snapshot.cpp
    cli/commands/set.cpp
    cli/commands/scan.cpp
    cli/commands/watch.cpp
    cli/commands/write.cpp
  utils/endianness.cpp
  utils/parserStr.cpp
  utils/sets.cpp
  utils/string.cpp
  utils/logging.cpp
  scanmem.cpp
)

# 查找 ICU
find_package(ICU REQUIRED COMPONENTS uc i18n)

target_compile_features(NewScanmem_lib PUBLIC cxx_std_23)

target_include_directories(NewScanmem_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

file(GLOB_RECURSE NEWSCANMEM_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/include/newscanmem/*.hpp")
target_sources(NewScanmem_lib PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_SOURCE_DIR}/include"
    FILES ${NEWSCANMEM_HEADERS})

target_link_libraries(NewScanmem_lib PUBLIC ICU::uc ICU::i18n)

# --- 主可执行文件 ---
add_executable(NewScanmem main.cpp)
target_link_libraries(NewScanmem PRIVATE NewScanmem_lib)
