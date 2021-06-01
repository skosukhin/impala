if(MSVC)
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_compile_options("/wd4800" "/wd4520")
    add_compile_options("/experimental:external" "/external:anglebrackets" "/external:W0")
else()
    add_compile_options("-Wall" "-Wextra")
endif()

find_package(Thorin REQUIRED)

# Enables '#include "impala/some_header.h"' when added to the header search
# paths before the package is installed:
set(IMPALA_SRC_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(impala)

if(Thorin_HAS_LLVM_SUPPORT)
    add_subdirectory(intrinsicgen)
endif()
