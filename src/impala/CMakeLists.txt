set(IMPALA_SOURCES
    args.h
    ast.cpp
    ast.h
    ast_stream.cpp
    cgen.cpp
    cgen.h
    emit.cpp
    impala.cpp
    impala.h
    lexer.cpp
    lexer.h
    parser.cpp
    sema/infersema.cpp
    sema/namesema.cpp
    sema/type.cpp
    sema/type.h
    sema/typesema.cpp
    token.cpp
    token.h
    tokenlist.h
)

# We currently assume that all '*.h' files in IMPALA_SOURCES are public headers:
set(IMPALA_PUBLIC_HEADERS ${IMPALA_SOURCES})
list(FILTER IMPALA_PUBLIC_HEADERS INCLUDE REGEX "\\.h$")

add_library(libimpala ${IMPALA_SOURCES})
set_target_properties(libimpala PROPERTIES PREFIX "")

target_include_directories(libimpala PUBLIC
    $<BUILD_INTERFACE:${IMPALA_SRC_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    ${Thorin_INCLUDE_DIRS}
)

# Thorin is exposed via the public headers of Impala, which triggers a direct
# dependency on Thorin library in a dependent package:
target_link_libraries(libimpala PUBLIC ${Thorin_LIBRARIES})

add_executable(impala main.cpp)

if(Thorin_HAS_LLVM_SUPPORT)
    target_compile_definitions(impala PRIVATE LLVM_SUPPORT)
endif()

target_link_libraries(impala PRIVATE
    libimpala
    # The executable has an explicit direct dependency on Thorin, therefore we
    # should specify ${Thorin_LIBRARIES} here even though it is also a public
    # dependency of libimpala:
    ${Thorin_LIBRARIES}
)

if(MSVC)
    set_target_properties(impala PROPERTIES LINK_FLAGS /STACK:8388608)
endif(MSVC)

# Install public headers preserving the directory layout:
foreach(header ${IMPALA_PUBLIC_HEADERS})
    get_filename_component(dir ${header} DIRECTORY)
    install(FILES ${header} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/impala/${dir})
endforeach()

# Install libraries and executables:
install(TARGETS libimpala impala
        EXPORT impala-targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
