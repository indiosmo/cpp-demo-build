cmake_minimum_required(VERSION 4.0)

project(matching_engine_lab
  VERSION 0.1.0
  DESCRIPTION "C++ matching engine laboratory"
  LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LAB_RELEASE          "Build with release flags and optimizations"  OFF)
option(LAB_DEBUG            "Build with debug-friendly settings"          OFF)
option(LAB_ASAN             "Build with Address/Leak/UB Sanitizer"        OFF)
option(LAB_TSAN             "Build with Thread Sanitizer"                 OFF)
option(LAB_BUILD_EXAMPLES   "Build illustrative example binaries"         OFF)

include(CompilerFlags)

# CTest defines BUILD_TESTING (default ON) and runs enable_testing() at the
# top level so add_test() in subdirectories is picked up.
include(CTest)

add_subdirectory(vendor)

# Project targets inherit lab::compiler_flags (warnings, -Werror, sanitizer
# wiring). Apply it only to directories added below this point so that
# third-party libraries pulled in by vendor/ via FetchContent are built with
# their own flags. Their export sets cannot reference lab::compiler_flags.
link_libraries(lab::compiler_flags)

add_subdirectory(src)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
