cmake_minimum_required(VERSION 3.22)

project(kraken
  VERSION 0.1.0
  DESCRIPTION "Kraken senior order book submission"
  LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(KRAKEN_RELEASE          "Build with release flags and optimizations"  OFF)
option(KRAKEN_DEBUG            "Build with debug-friendly settings"          OFF)
option(KRAKEN_ASAN             "Build with Address/Leak/UB Sanitizer"        OFF)
option(KRAKEN_TSAN             "Build with Thread Sanitizer"                 OFF)
option(KRAKEN_BUILD_BENCHMARKS "Build google-benchmark microbenchmarks"      ON)

include(CompilerFlags)

# CTest defines BUILD_TESTING (default ON) and runs enable_testing() at the
# top level so add_test() in subdirectories is picked up.
include(CTest)

add_subdirectory(submission)
