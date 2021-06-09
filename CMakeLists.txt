cmake_minimum_required(VERSION 3.13.4 FATAL_ERROR)

project(Impala)

set(PACKAGE_VERSION "0.3.9")
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limited config" FORCE)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Options and option-like cache variables:
#
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

set(CMAKE_POSITION_INDEPENDENT_CODE FALSE CACHE BOOL "Set to true to enable PIC in static libraries")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Debug or Release" FORCE)
elseif(NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND
       NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    message(FATAL_ERROR "CMAKE_BUILD_TYPE is not recognized: ${CMAKE_BUILD_TYPE}")
endif()

#
# Dependencies:
#
find_package(Thorin REQUIRED)

if(BUILD_TESTING)
    find_package(PythonInterp 3.0 REQUIRED)
    set(PYTHON_BIN ${PYTHON_EXECUTABLE})

    find_package(LLVM QUIET CONFIG PATHS ${LLVM_DIR} $ENV{LLVM_DIR})
    find_package(Clang QUIET CONFIG PATHS ${Clang_DIR} ${LLVM_DIR} PATH_SUFFIXES ../clang ../lib/cmake/clang)
    if(TARGET clang AND NOT Clang_BIN)
        get_target_property(Clang_BIN clang LOCATION)
        set(Clang_BIN ${Clang_BIN} CACHE FILEPATH "path to clang executable")
    endif()
    find_program(Clang_BIN clang
        PATHS ${LLVM_TOOLS_BINARY_DIR} ${LLVM_INSTALL_PREFIX}/bin
        PATH_SUFFIXES Release ${CMAKE_CONFIGURATION_TYPES}
    )
endif()

#
# Build type and common flags:
#
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Using Debug flags: ${CMAKE_CXX_FLAGS_DEBUG}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Using Release flags: ${CMAKE_CXX_FLAGS_RELEASE}")
endif()

#
# Build and installation directory layout:
#
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(GNUInstallDirs)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR})
add_subdirectory(src)

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()

# Export target files
#   (enables find_package(Impala) before the package is installed):
export(EXPORT impala-targets
       NAMESPACE "AnyDSL::"
       FILE ${CMAKE_BINARY_DIR}/share/anydsl/cmake/impala-targets.cmake
)

# Generate impala-config.cmake:
include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/impala-config.cmake.in
    ${CMAKE_BINARY_DIR}/share/anydsl/cmake/impala-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/impala/cmake
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

# Generate impala-config-version.cmake
#   (enables find_package(Impala X.Y.Z)
write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/share/anydsl/cmake/impala-config-version.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Install the config files
#   (the destination enables find_package(Impala) with
#    CMAKE_PREFIX_PATH=/prefix/to/impala):
install(FILES
    ${CMAKE_BINARY_DIR}/share/anydsl/cmake/impala-config.cmake
    ${CMAKE_BINARY_DIR}/share/anydsl/cmake/impala-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/impala/cmake
)

# Install the exported target files
#   (enables find_package(Impala) after the package is installed):
install(EXPORT impala-targets
        NAMESPACE "AnyDSL::"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/impala/cmake
)
