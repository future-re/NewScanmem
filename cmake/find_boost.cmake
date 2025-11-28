# find_boost.cmake — 如果系统未提供 Boost，则下载源码并用本机编译器构建（stage），创建导入目标

# 优先尝试系统安装的 Boost（会创建 Boost::filesystem / Boost::regex 导入目标）
find_package(Boost 1.74.0 QUIET COMPONENTS filesystem regex)

if(NOT Boost_FOUND)
    message(STATUS "System Boost not found. Will fetch and build Boost 1.89 from source (cached).")

    include(FetchContent)

    # 显示下载进度
    set(FETCHCONTENT_QUIET OFF)
    set(FETCHCONTENT_PROGRESS_UPDATE_INTERVAL 1)

    FetchContent_Declare(
        Boost
        URL https://archives.boost.io/release/1.89.0/source/boost_1_89_0.tar.gz
        URL_HASH SHA256=9de758db755e8330a01d995b0a24d09798048400ac25c03fc5ea9be364b13c93
    )

    message(STATUS "Fetching Boost sources...")
    # 仅下载并解压源码，不自动 add_subdirectory
    FetchContent_Populate(Boost)

    if(NOT boost_SOURCE_DIR)
        message(FATAL_ERROR "boost_SOURCE_DIR not set after FetchContent_Populate. Aborting.")
    endif()

    # stage 目录（b2 输出）
    set(_stage_lib_dir "${boost_SOURCE_DIR}/stage/lib")

    # 如果 stage 已包含我们需要的库则复用，避免重复构建
    find_library(_found_fs NAMES boost_filesystem libboost_filesystem boost_filesystem-mt libboost_filesystem-mt HINTS ${_stage_lib_dir})
    find_library(_found_rg NAMES boost_regex libboost_regex boost_regex-mt libboost_regex-mt HINTS ${_stage_lib_dir})

    if(_found_fs AND _found_rg)
        message(STATUS "Found Boost libraries in ${_stage_lib_dir}, skipping build.")
    else()
        message(STATUS "Building Boost (filesystem, regex) in ${boost_SOURCE_DIR} — this may take several minutes...")

        # 优先使用环境变量 CC/CXX；否则尝试查找 clang-20/clang++-20，最后退回 clang/clang++
        if(DEFINED ENV{CC})
            set(_cc_env $ENV{CC})
        else()
            find_program(_cc_env clang-20 clang clang-11 clang-12)
        endif()

        if(DEFINED ENV{CXX})
            set(_cxx_env $ENV{CXX})
        else()
            find_program(_cxx_env clang++-20 clang++ clang++-11 clang++-12)
        endif()

        if(NOT _cc_env OR NOT _cxx_env)
            message(FATAL_ERROR "C/C++ compiler for building Boost not found. Set CC and CXX environment variables or install clang-20.")
        endif()

        # 并行度
        execute_process(COMMAND nproc OUTPUT_VARIABLE _nproc OUTPUT_STRIP_TRAILING_WHITESPACE)
        if(NOT _nproc)
            set(_nproc 1)
        endif()

        # 生成 b2
        execute_process(
            COMMAND ./bootstrap.sh
            WORKING_DIRECTORY ${boost_SOURCE_DIR}
            RESULT_VARIABLE _bs_res
            OUTPUT_VARIABLE _bs_out
            ERROR_VARIABLE _bs_err
            TIMEOUT 300
        )
        if(NOT _bs_res EQUAL 0)
            message(FATAL_ERROR "Boost bootstrap.sh failed:\n${_bs_err}\n${_bs_out}")
        endif()

        # b2 stage 到源码目录下的 stage，传入 CC/CXX，且并行构建只包含需要的库
        execute_process(
            COMMAND ./b2 --with-filesystem --with-regex -j${_nproc} stage
            WORKING_DIRECTORY ${boost_SOURCE_DIR}
            RESULT_VARIABLE _b2_res
            OUTPUT_VARIABLE _b2_out
            ERROR_VARIABLE _b2_err
            TIMEOUT 0
            ENVIRONMENT "CC=${_cc_env}" "CXX=${_cxx_env}"
        )
        if(NOT _b2_res EQUAL 0)
            message(FATAL_ERROR "Boost b2 stage failed:\n${_b2_err}\n${_b2_out}")
        endif()

        # 尝试再次查找构建产物
        find_library(_found_fs NAMES boost_filesystem libboost_filesystem boost_filesystem-mt libboost_filesystem-mt HINTS ${_stage_lib_dir})
        find_library(_found_rg NAMES boost_regex libboost_regex boost_regex-mt libboost_regex-mt HINTS ${_stage_lib_dir})

        if(NOT _found_fs OR NOT _found_rg)
            message(FATAL_ERROR "Built Boost libraries not found in ${_stage_lib_dir} after b2.")
        endif()
    endif()

    # 设置包含和库路径（头文件仍在源码树）
    set(Boost_INCLUDE_DIRS "${boost_SOURCE_DIR}" CACHE INTERNAL "Boost include directories")
    set(Boost_LIBRARY_DIRS "${_stage_lib_dir}" CACHE INTERNAL "Boost library directories")

    # 最终库路径变量
    find_library(BOOST_FILESYSTEM_LIB NAMES boost_filesystem libboost_filesystem boost_filesystem-mt libboost_filesystem-mt HINTS ${_stage_lib_dir})
    find_library(BOOST_REGEX_LIB      NAMES boost_regex libboost_regex boost_regex-mt libboost_regex-mt HINTS ${_stage_lib_dir})

    if(NOT BOOST_FILESYSTEM_LIB OR NOT BOOST_REGEX_LIB)
        message(FATAL_ERROR "Failed to locate built Boost libraries in ${_stage_lib_dir}")
    endif()

    # 创建导入目标以便主项目直接使用 Boost::filesystem / Boost::regex
    if(NOT TARGET Boost::filesystem)
        add_library(Boost::filesystem UNKNOWN IMPORTED)
        set_target_properties(Boost::filesystem PROPERTIES
            IMPORTED_LOCATION "${BOOST_FILESYSTEM_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIRS}"
        )
    endif()

    if(NOT TARGET Boost::regex)
        add_library(Boost::regex UNKNOWN IMPORTED)
        set_target_properties(Boost::regex PROPERTIES
            IMPORTED_LOCATION "${BOOST_REGEX_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${Boost_INCLUDE_DIRS}"
        )
    endif()

    set(Boost_LIBRARIES Boost::filesystem Boost::regex CACHE INTERNAL "Boost libraries")
    set(Boost_FOUND TRUE CACHE INTERNAL "Boost found (built from source)")

else()
    message(STATUS "Found Boost ${Boost_VERSION} in system; using system targets.")
    set(Boost_LIBRARIES Boost::filesystem Boost::regex)
endif()