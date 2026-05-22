cmake_minimum_required(VERSION 4.0)

# When built standalone (e.g. inside the Dockerfile pipeline) this directory
# is the top-level project; otherwise the repository root drives the build.
if(NOT PROJECT_NAME)
  project(kraken VERSION 0.1.0 LANGUAGES CXX)

  # Catch2-based unit tests live under the parent project; the docker pipeline
  # only needs the kraken_submission binary, so skip them when standalone.
  set(BUILD_TESTING OFF)
endif()

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(KRAKEN_BUILD_EXAMPLES "Build illustrative example binaries" OFF)

add_subdirectory(vendor)
add_subdirectory(src)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()

if(KRAKEN_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()
