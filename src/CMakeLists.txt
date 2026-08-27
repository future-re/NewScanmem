# --- Source files ---
file(GLOB_RECURSE NEWSCANMEM_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

add_library(NewScanmem_lib STATIC ${NEWSCANMEM_SOURCES})

# 查找 ICU
find_package(ICU REQUIRED COMPONENTS uc i18n)

target_compile_features(NewScanmem_lib PUBLIC cxx_std_23)

target_include_directories(NewScanmem_lib PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)

# Header files for installation
file(GLOB_RECURSE NEWSCANMEM_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/include/newscanmem/*.hpp")
target_sources(NewScanmem_lib PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_SOURCE_DIR}/include"
    FILES ${NEWSCANMEM_HEADERS})

target_link_libraries(NewScanmem_lib PUBLIC ICU::uc ICU::i18n)

# --- Main executable ---
add_executable(NewScanmem main.cpp)
target_link_libraries(NewScanmem PRIVATE NewScanmem_lib)
