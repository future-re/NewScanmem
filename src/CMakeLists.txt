# --- Source files ---
file(GLOB_RECURSE MEMSEEK_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# These components are being refactored and are intentionally left out of the
# current build. Keep the filter here so newly added files in those folders do
# not get picked up by the recursive glob automatically.
list(FILTER MEMSEEK_SOURCES EXCLUDE REGEX
    "/(app|cli|core|generated|ui|utils)/")

add_library(memseek_lib STATIC ${MEMSEEK_SOURCES})

# 查找 ICU
find_package(ICU REQUIRED COMPONENTS uc i18n)

target_compile_features(memseek_lib PUBLIC cxx_std_23)

target_include_directories(memseek_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

# Header files for installation
file(GLOB_RECURSE MEMSEEK_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/include/memseek/*.hpp")
list(FILTER MEMSEEK_HEADERS EXCLUDE REGEX
    "/(app|cli|core|generated|ui|utils)/")
target_sources(memseek_lib PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_SOURCE_DIR}/include"
    FILES ${MEMSEEK_HEADERS})

target_link_libraries(memseek_lib PUBLIC ICU::uc ICU::i18n)

# --- Main executable ---
add_executable(memseek main.cpp)
target_link_libraries(memseek PRIVATE memseek_lib)
