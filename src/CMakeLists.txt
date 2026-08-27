# --- Source files ---
file(GLOB_RECURSE NEWSCANMEM_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(memseek_lib STATIC ${NEWSCANMEM_SOURCES})

# 查找 ICU
find_package(ICU REQUIRED COMPONENTS uc i18n)

target_compile_features(memseek_lib PUBLIC cxx_std_23)

target_include_directories(memseek_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

# Header files for installation
file(GLOB_RECURSE MEMSEEK_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/include/newscanmem/*.hpp")
target_sources(memseek_lib PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_SOURCE_DIR}/include"
    FILES ${MEMSEEK_HEADERS})

target_link_libraries(memseek_lib PUBLIC ICU::uc ICU::i18n)

# --- Main executable ---
add_executable(memseek main.cpp)
target_link_libraries(memseek PRIVATE memseek_lib)
