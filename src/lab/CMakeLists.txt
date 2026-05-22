find_package(Boost REQUIRED)
find_package(fmt REQUIRED)

add_library(lab_core INTERFACE)
add_library(lab::core ALIAS lab_core)

target_include_directories(lab_core
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)

# Boost is consumed header-only: leaf (result/error transport), container_hash
# (fixed_string hashing), pfr (lab::auto_hash reflective field walk),
# algorithm (string replace in result.hpp's leaf diagnostic formatter), and
# stacktrace (LAB_ASSERT, header-only mode -- raw addresses, no symbol
# resolution).
#
# fmt is taken from the system libfmt-dev as a header-only interface target,
# so call sites inline through the same fmt ABI as libspdlog-dev.
#
# Vendor targets back the lab vocabulary headers. Consumers should link only
# lab::core and inherit the vendored implementation details transitively.
target_link_libraries(lab_core
  INTERFACE
    Boost::headers
    fmt::fmt-header-only
    lab::vendor::inplace_function
    lab::vendor::NamedType
    lab::vendor::readerwriterqueue
)

add_subdirectory(libs)
