cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

# add_library(rtmock STATIC rtmock.cpp)

set(TEST_SCRIPT perform.py)
set(TEST_ARGS --impala $<TARGET_FILE:impala> --clang ${Clang_BIN} --temp ${CMAKE_CURRENT_BINARY_DIR} --rtmock "${CMAKE_CURRENT_SOURCE_DIR}/rtmock.cpp")

file(GLOB_RECURSE _testcases RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.impala")

foreach(_test ${_testcases})
    add_test(NAME ${_test} COMMAND ${PYTHON_BIN} ${TEST_SCRIPT} ${TEST_ARGS} ${_test} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    set_tests_properties(${_test} PROPERTIES SKIP_RETURN_CODE 77)
endforeach()

set(_content
    "CONFIGURATION = \"$<CONFIG>\"\nIMPALA_BIN = \"$<TARGET_FILE:impala>\"\nCLANG_BIN = \"${Clang_BIN}\"\nLIBRTMOCK = \"${CMAKE_CURRENT_SOURCE_DIR}/rtmock.cpp\"\nTEMP_DIR = \"${CMAKE_CURRENT_BINARY_DIR}\"\n")
file(GENERATE OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/config$<CONFIG>.py CONTENT ${_content})
