cmake_minimum_required(VERSION 3.13.4 FATAL_ERROR)

project(Impala)

set(PACKAGE_VERSION "0.3.9")
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limited config" FORCE)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)

if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Debug or Release" FORCE)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(GNUInstallDirs)
add_subdirectory(src)

message(STATUS "Using Debug flags: ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "Using Release flags: ${CMAKE_CXX_FLAGS_RELEASE}")
if(DEFINED CMAKE_BUILD_TYPE)
    message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
endif()

if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()

# Export target files
#   (enables find_package(Impala) before the package is installed):
export(EXPORT impala-targets
       NAMESPACE "Impala::"
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
        NAMESPACE "Impala::"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/impala/cmake
)
