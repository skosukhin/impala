add_executable(intrinsicgen main.cpp)

# The executable does not have an explicit direct dependency on Thorin, however,
# we have to put ${Thorin_LIBRARIES} before libimpala because otherwise, the
# produced executable crashes somewhere inside thorin/util/hash.h when both
# libraries are static.
target_link_libraries(intrinsicgen PRIVATE ${Thorin_LIBRARIES} libimpala)

# The executable has an explicit direct dependency on LLVM, which we reflect
# here even though LLVM is also a public dependency of libimpala.
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
target_compile_definitions(intrinsicgen PRIVATE ${LLVM_DEFINITIONS_LIST})
target_include_directories(intrinsicgen SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS})
llvm_config(intrinsicgen ${AnyDSL_LLVM_LINK_SHARED} core support)

# Install the executable:
install(TARGETS intrinsicgen
        EXPORT impala-targets
        DESTINATION ${CMAKE_INSTALL_BINDIR}
)
