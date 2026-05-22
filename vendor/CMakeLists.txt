# Third-party dependencies. Every dependency reaches the build through
# FetchContent from a public GitHub upstream pinned by tag or SHA, per
# ADR 0002 (docs/adr/0002-use-fetchcontent-for-third-party-dependencies.md).
#
# Boost and the C++26 standard library are toolchain-provided and not
# declared here -- see scripts/dependencies/install_boost.sh and the
# BOOST_ROOT path loading in scripts/setpath.sh.

include(FetchContent)

set(FETCHCONTENT_QUIET OFF)

# Header-only libraries of types. The upstream CMake builds tests we do
# not want, so SOURCE_SUBDIR points at a directory that does not exist;
# FetchContent_MakeAvailable then populates the source without calling
# add_subdirectory, and we declare our own INTERFACE wrapper aliased
# under lab::vendor::<name>.

FetchContent_Declare(NamedType
  GIT_REPOSITORY https://github.com/joboccara/NamedType.git
  GIT_TAG        76668abe09807f92a695ee5e868f9719e888e65f
  GIT_SHALLOW    TRUE
  SOURCE_SUBDIR  _lab_skip_add_subdirectory_
  SYSTEM
)

FetchContent_Declare(SG14
  GIT_REPOSITORY https://github.com/WG21-SG14/SG14.git
  GIT_TAG        c92614381100bd23f3f0de7882aa455797255c03
  GIT_SHALLOW    TRUE
  SOURCE_SUBDIR  _lab_skip_add_subdirectory_
  SYSTEM
)

FetchContent_Declare(readerwriterqueue
  GIT_REPOSITORY https://github.com/cameron314/readerwriterqueue.git
  GIT_TAG        v1.0.6
  GIT_SHALLOW    TRUE
  SOURCE_SUBDIR  _lab_skip_add_subdirectory_
  SYSTEM
)

FetchContent_MakeAvailable(NamedType SG14 readerwriterqueue)

add_library(lab_vendor_NamedType INTERFACE)
add_library(lab::vendor::NamedType ALIAS lab_vendor_NamedType)
target_include_directories(lab_vendor_NamedType SYSTEM INTERFACE
  ${namedtype_SOURCE_DIR}/include)

add_library(lab_vendor_inplace_function INTERFACE)
add_library(lab::vendor::inplace_function ALIAS lab_vendor_inplace_function)
target_include_directories(lab_vendor_inplace_function SYSTEM INTERFACE
  ${sg14_SOURCE_DIR})

add_library(lab_vendor_readerwriterqueue INTERFACE)
add_library(lab::vendor::readerwriterqueue ALIAS lab_vendor_readerwriterqueue)
target_include_directories(lab_vendor_readerwriterqueue SYSTEM INTERFACE
  ${readerwriterqueue_SOURCE_DIR})

set(JSON_BuildTests OFF CACHE INTERNAL "")
FetchContent_Declare(nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.12.0
  GIT_SHALLOW    TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(nlohmann_json)

# Compiled libraries of functions. The upstream CMake defines the
# canonical target (fmt::fmt-header-only, spdlog::spdlog,
# Catch2::Catch2WithMain); consumers link those names directly. Tests,
# examples, docs, and install rules are disabled per dependency to keep them
# out of this build.

set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
set(FMT_TEST    OFF CACHE BOOL "" FORCE)
set(FMT_DOC     OFF CACHE BOOL "" FORCE)
set(FMT_FUZZ    OFF CACHE BOOL "" FORCE)
FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG        11.0.2
  GIT_SHALLOW    TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(fmt)

# spdlog uses fmt::fmt-header-only when SPDLOG_FMT_EXTERNAL_HO is on, so
# both spdlog and the lab vocabulary share one fmt ABI.
set(SPDLOG_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE  OFF CACHE BOOL "" FORCE)
set(SPDLOG_INSTALL        OFF CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL_HO ON CACHE BOOL "" FORCE)
FetchContent_Declare(spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG        v1.14.1
  GIT_SHALLOW    TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(spdlog)

set(CATCH_INSTALL_DOCS   OFF CACHE BOOL "" FORCE)
set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)
set(CATCH_BUILD_TESTING  OFF CACHE BOOL "" FORCE)
FetchContent_Declare(Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.7.1
  GIT_SHALLOW    TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(Catch2)

# Catch2 ships a CMake helper (Catch.cmake) that defines
# catch_discover_tests; expose it on the module path so test/CMakeLists.txt
# can include(Catch).
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" PARENT_SCOPE)

# QuickFIX exposes a single `quickfix` target (upstream provides no namespace
# alias). Force a static build so engine binaries do not need libquickfix.so
# on LD_LIBRARY_PATH at runtime. QUICKFIX_LIB_OUTPUT_DIR defaults to
# ${CMAKE_SOURCE_DIR}/lib, which would land inside this repo's source tree --
# redirect into the build tree.
set(QUICKFIX_EXAMPLES    OFF CACHE BOOL "" FORCE)
set(QUICKFIX_TESTS       OFF CACHE BOOL "" FORCE)
set(QUICKFIX_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(QUICKFIX_LIB_OUTPUT_DIR "${CMAKE_BINARY_DIR}/quickfix-lib" CACHE PATH "" FORCE)
FetchContent_Declare(quickfix
  GIT_REPOSITORY https://github.com/quickfix/quickfix.git
  GIT_TAG        v1.16.0
  GIT_SHALLOW    TRUE
  SYSTEM
  EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(quickfix)
