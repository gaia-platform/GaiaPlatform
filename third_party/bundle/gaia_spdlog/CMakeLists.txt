# Copyright(c) 2019 spdlog authors Distributed under the MIT License (http://opensource.org/licenses/MIT)

cmake_minimum_required(VERSION 3.10)

# ---------------------------------------------------------------------------------------
# Start gaia_spdlog project
# ---------------------------------------------------------------------------------------
include(cmake/utils.cmake)
include(cmake/ide.cmake)

spdlog_extract_version()

project(gaia_spdlog VERSION ${GAIA_SPDLOG_VERSION} LANGUAGES CXX)
message(STATUS "Build gaia_spdlog: ${GAIA_SPDLOG_VERSION}")

include(GNUInstallDirs)

# ---------------------------------------------------------------------------------------
# Set default build to release
# ---------------------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()

# ---------------------------------------------------------------------------------------
# Compiler config
# ---------------------------------------------------------------------------------------
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# make sure __cplusplus is defined when using msvc
if(MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus")
endif()

set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_SYSTEM_NAME MATCHES "CYGWIN" OR CMAKE_SYSTEM_NAME MATCHES "MSYS")
    set(CMAKE_CXX_EXTENSIONS ON)
endif()

# ---------------------------------------------------------------------------------------
# Set GAIA_SPDLOG_MASTER_PROJECT to ON if we are building gaia_spdlog
# ---------------------------------------------------------------------------------------
# Check if gaia_spdlog is being used directly or via add_subdirectory, but allow overriding
if(NOT DEFINED GAIA_SPDLOG_MASTER_PROJECT)
    if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
        set(GAIA_SPDLOG_MASTER_PROJECT ON)
    else()
        set(GAIA_SPDLOG_MASTER_PROJECT OFF)
    endif()
endif()

option(GAIA_SPDLOG_BUILD_ALL "Build all artifacts" OFF)

# build shared option
option(GAIA_SPDLOG_BUILD_SHARED "Build shared library" OFF)

# precompiled headers option
option(GAIA_SPDLOG_ENABLE_PCH "Build static or shared library using precompiled header to speed up compilation time" OFF)

# example options
option(GAIA_SPDLOG_BUILD_EXAMPLE "Build example" ${GAIA_SPDLOG_MASTER_PROJECT})
option(GAIA_SPDLOG_BUILD_EXAMPLE_HO "Build header only example" OFF)

# testing options
option(GAIA_SPDLOG_BUILD_TESTS "Build tests" OFF)
option(GAIA_SPDLOG_BUILD_TESTS_HO "Build tests using the header only version" OFF)

# bench options
option(GAIA_SPDLOG_BUILD_BENCH "Build benchmarks (Requires https://github.com/google/benchmark.git to be installed)" OFF)

# sanitizer options
option(GAIA_SPDLOG_SANITIZE_ADDRESS "Enable address sanitizer in tests" OFF)

# warning options
option(GAIA_SPDLOG_BUILD_WARNINGS "Enable compiler warnings" OFF)

# install options
option(GAIA_SPDLOG_INSTALL "Generate the install target" ${GAIA_SPDLOG_MASTER_PROJECT})
option(GAIA_SPDLOG_FMT_EXTERNAL "Use external fmt library instead of bundled" OFF)
option(GAIA_SPDLOG_FMT_EXTERNAL_HO "Use external fmt header-only library instead of bundled" OFF)
option(GAIA_SPDLOG_NO_EXCEPTIONS "Compile with -fno-exceptions. Call abort() on any gaia_spdlog exceptions" OFF)

if(GAIA_SPDLOG_FMT_EXTERNAL AND GAIA_SPDLOG_FMT_EXTERNAL_HO)
    message(FATAL_ERROR "GAIA_SPDLOG_FMT_EXTERNAL and GAIA_SPDLOG_FMT_EXTERNAL_HO are mutually exclusive")
endif()

# misc tweakme options
if(WIN32)
    option(GAIA_SPDLOG_WCHAR_SUPPORT "Support wchar api" OFF)
    option(GAIA_SPDLOG_WCHAR_FILENAMES "Support wchar filenames" OFF)
endif()
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
    option(GAIA_SPDLOG_CLOCK_COARSE
           "Use the much faster (but much less accurate) CLOCK_REALTIME_COARSE instead of the regular clock," OFF)
endif()

option(GAIA_SPDLOG_PREVENT_CHILD_FD "Prevent from child processes to inherit log file descriptors" OFF)
option(GAIA_SPDLOG_NO_THREAD_ID "prevent gaia_spdlog from querying the thread id on each log call if thread id is not needed" OFF)
option(GAIA_SPDLOG_NO_TLS "prevent gaia_spdlog from using thread local storage" OFF)
option(
    GAIA_SPDLOG_NO_ATOMIC_LEVELS
    "prevent gaia_spdlog from using of std::atomic log levels (use only if your code never modifies log levels concurrently"
    OFF)
option(GAIA_SPDLOG_DISABLE_DEFAULT_LOGGER "Disable default logger creation" OFF)

# clang-tidy
if(${CMAKE_VERSION} VERSION_GREATER "3.5")
    option(GAIA_SPDLOG_TIDY "run clang-tidy" OFF)
endif()

if(GAIA_SPDLOG_TIDY)
    set(CMAKE_CXX_CLANG_TIDY "clang-tidy")
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    message(STATUS "Enabled clang-tidy")
endif()

find_package(Threads REQUIRED)
message(STATUS "Build type: " ${CMAKE_BUILD_TYPE})
# ---------------------------------------------------------------------------------------
# Static/Shared library (shared not supported in windows yet)
# ---------------------------------------------------------------------------------------
set(GAIA_SPDLOG_SRCS src/gaia_spdlog.cpp src/stdout_sinks.cpp src/color_sinks.cpp src/file_sinks.cpp src/async.cpp src/cfg.cpp)

if(NOT GAIA_SPDLOG_FMT_EXTERNAL AND NOT GAIA_SPDLOG_FMT_EXTERNAL_HO)
    list(APPEND GAIA_SPDLOG_SRCS src/fmt.cpp)
endif()

if(GAIA_SPDLOG_BUILD_SHARED OR BUILD_SHARED_LIBS)
    if(WIN32)
        configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)
        list(APPEND GAIA_SPDLOG_SRCS ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
    endif()
    add_library(gaia_spdlog SHARED ${GAIA_SPDLOG_SRCS} ${GAIA_SPDLOG_ALL_HEADERS})
    target_compile_definitions(gaia_spdlog PUBLIC GAIA_SPDLOG_SHARED_LIB)
    if(MSVC)
        target_compile_options(gaia_spdlog PUBLIC
            $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<NOT:$<COMPILE_LANGUAGE:CUDA>>>:/wd4251 /wd4275>)
    endif()
    if(NOT GAIA_SPDLOG_FMT_EXTERNAL AND NOT GAIA_SPDLOG_FMT_EXTERNAL_HO)
        target_compile_definitions(gaia_spdlog PRIVATE GAIA_FMT_EXPORT PUBLIC GAIA_FMT_SHARED)
    endif()
else()
    add_library(gaia_spdlog STATIC ${GAIA_SPDLOG_SRCS} ${GAIA_SPDLOG_ALL_HEADERS})
endif()

add_library(gaia_spdlog::gaia_spdlog ALIAS gaia_spdlog)

target_compile_definitions(gaia_spdlog PUBLIC GAIA_SPDLOG_COMPILED_LIB)
target_include_directories(gaia_spdlog SYSTEM PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
                                         "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(gaia_spdlog PUBLIC Threads::Threads)
spdlog_enable_warnings(gaia_spdlog)

set_target_properties(gaia_spdlog PROPERTIES VERSION ${GAIA_SPDLOG_VERSION} SOVERSION ${GAIA_SPDLOG_VERSION_MAJOR})
set_target_properties(gaia_spdlog PROPERTIES DEBUG_POSTFIX d)

if(COMMAND target_precompile_headers AND GAIA_SPDLOG_ENABLE_PCH)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/pch.h.in ${PROJECT_BINARY_DIR}/gaia_spdlog_pch.h @ONLY)
    target_precompile_headers(gaia_spdlog PRIVATE ${PROJECT_BINARY_DIR}/gaia_spdlog_pch.h)
endif()

# ---------------------------------------------------------------------------------------
# Header only version
# ---------------------------------------------------------------------------------------
add_library(gaia_spdlog_header_only INTERFACE)
add_library(gaia_spdlog::gaia_spdlog_header_only ALIAS gaia_spdlog_header_only)

target_include_directories(gaia_spdlog_header_only SYSTEM INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>"
                                                        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(gaia_spdlog_header_only INTERFACE Threads::Threads)

# ---------------------------------------------------------------------------------------
# Use fmt package if using external fmt
# ---------------------------------------------------------------------------------------
if(GAIA_SPDLOG_FMT_EXTERNAL OR GAIA_SPDLOG_FMT_EXTERNAL_HO)
    if(NOT TARGET fmt::fmt)
        find_package(fmt CONFIG REQUIRED)
    endif()
    target_compile_definitions(gaia_spdlog PUBLIC GAIA_SPDLOG_FMT_EXTERNAL)
    target_compile_definitions(gaia_spdlog_header_only INTERFACE GAIA_SPDLOG_FMT_EXTERNAL)

    # use external fmt-header-nly
    if(GAIA_SPDLOG_FMT_EXTERNAL_HO)
        target_link_libraries(gaia_spdlog PUBLIC fmt::fmt-header-only)
        target_link_libraries(gaia_spdlog_header_only INTERFACE fmt::fmt-header-only)
    else() # use external compile fmt
        target_link_libraries(gaia_spdlog PUBLIC fmt::fmt)
        target_link_libraries(gaia_spdlog_header_only INTERFACE fmt::fmt)
    endif()

    set(PKG_CONFIG_REQUIRES fmt) # add dependency to pkg-config
endif()

# ---------------------------------------------------------------------------------------
# Add required libraries for Android CMake build
# ---------------------------------------------------------------------------------------
if (ANDROID)
    target_link_libraries(gaia_spdlog PUBLIC log)
    target_link_libraries(gaia_spdlog_header_only INTERFACE log)
endif ()

# ---------------------------------------------------------------------------------------
# Misc definitions according to tweak options
# ---------------------------------------------------------------------------------------
set(GAIA_SPDLOG_WCHAR_TO_UTF8_SUPPORT ${GAIA_SPDLOG_WCHAR_SUPPORT})
foreach(
    GAIA_SPDLOG_OPTION
    GAIA_SPDLOG_WCHAR_TO_UTF8_SUPPORT
    GAIA_SPDLOG_WCHAR_FILENAMES
    GAIA_SPDLOG_NO_EXCEPTIONS
    GAIA_SPDLOG_CLOCK_COARSE
    GAIA_SPDLOG_PREVENT_CHILD_FD
    GAIA_SPDLOG_NO_THREAD_ID
    GAIA_SPDLOG_NO_TLS
    GAIA_SPDLOG_NO_ATOMIC_LEVELS
    GAIA_SPDLOG_DISABLE_DEFAULT_LOGGER)
    if(${GAIA_SPDLOG_OPTION})
        target_compile_definitions(gaia_spdlog PUBLIC ${GAIA_SPDLOG_OPTION})
        target_compile_definitions(gaia_spdlog_header_only INTERFACE ${GAIA_SPDLOG_OPTION})
    endif()
endforeach()

if(GAIA_SPDLOG_NO_EXCEPTIONS AND NOT MSVC)
    target_compile_options(gaia_spdlog PRIVATE -fno-exceptions)
endif()

# ---------------------------------------------------------------------------------------
# Build binaries
# ---------------------------------------------------------------------------------------
#if(GAIA_SPDLOG_BUILD_EXAMPLE OR GAIA_SPDLOG_BUILD_EXAMPLE_HO OR GAIA_SPDLOG_BUILD_ALL)
#    message(STATUS "Generating example(s)")
#    add_subdirectory(example)
#    spdlog_enable_warnings(example)
#    if(GAIA_SPDLOG_BUILD_EXAMPLE_HO)
#        spdlog_enable_warnings(example_header_only)
#    endif()
#endif()

if(GAIA_SPDLOG_BUILD_TESTS OR GAIA_SPDLOG_BUILD_TESTS_HO OR GAIA_SPDLOG_BUILD_ALL)
    message(STATUS "Generating tests")
    enable_testing()
    add_subdirectory(tests)
endif()

if(GAIA_SPDLOG_BUILD_BENCH OR GAIA_SPDLOG_BUILD_ALL)
    message(STATUS "Generating benchmarks")
    add_subdirectory(bench)
endif()

# ---------------------------------------------------------------------------------------
# Install
# ---------------------------------------------------------------------------------------
if(GAIA_SPDLOG_INSTALL)
    message(STATUS "Generating install")
    set(project_config_in "${CMAKE_CURRENT_LIST_DIR}/cmake/gaia_spdlogConfig.cmake.in")
    set(project_config_out "${CMAKE_CURRENT_BINARY_DIR}/gaia_spdlogConfig.cmake")
    set(config_targets_file "gaia_spdlogConfigTargets.cmake")
    set(version_config_file "${CMAKE_CURRENT_BINARY_DIR}/gaia_spdlogConfigVersion.cmake")
    set(export_dest_dir "${CMAKE_INSTALL_LIBDIR}/cmake/gaia_spdlog")
    set(pkgconfig_install_dir "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    set(pkg_config "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc")

    # ---------------------------------------------------------------------------------------
    # Include files
    # ---------------------------------------------------------------------------------------
    install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}" PATTERN "fmt/bundled" EXCLUDE)
    install(
        TARGETS gaia_spdlog gaia_spdlog_header_only
        EXPORT gaia_spdlog
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    if(NOT GAIA_SPDLOG_FMT_EXTERNAL AND NOT GAIA_SPDLOG_FMT_EXTERNAL_HO)
        install(DIRECTORY include/${PROJECT_NAME}/fmt/bundled/
                DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/fmt/bundled/")
    endif()

    # ---------------------------------------------------------------------------------------
    # Install pkg-config file
    # ---------------------------------------------------------------------------------------
    get_target_property(PKG_CONFIG_DEFINES gaia_spdlog INTERFACE_COMPILE_DEFINITIONS)
    string(REPLACE ";" " -D" PKG_CONFIG_DEFINES "${PKG_CONFIG_DEFINES}")
    string(CONCAT PKG_CONFIG_DEFINES "-D" "${PKG_CONFIG_DEFINES}")
    configure_file("cmake/${PROJECT_NAME}.pc.in" "${pkg_config}" @ONLY)
    install(FILES "${pkg_config}" DESTINATION "${pkgconfig_install_dir}")

    # ---------------------------------------------------------------------------------------
    # Install CMake config files
    # ---------------------------------------------------------------------------------------
    install(EXPORT gaia_spdlog DESTINATION ${export_dest_dir} NAMESPACE gaia_spdlog:: FILE ${config_targets_file})

    include(CMakePackageConfigHelpers)
    configure_file("${project_config_in}" "${project_config_out}" @ONLY)

    write_basic_package_version_file("${version_config_file}" COMPATIBILITY SameMajorVersion)
    install(FILES "${project_config_out}" "${version_config_file}" DESTINATION "${export_dest_dir}")

    # ---------------------------------------------------------------------------------------
    # Support creation of installable packages
    # ---------------------------------------------------------------------------------------
    include(cmake/spdlogCPack.cmake)
endif()
