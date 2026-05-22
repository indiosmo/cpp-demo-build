find_package(Boost REQUIRED)

add_library(lab_core INTERFACE)
add_library(lab::core ALIAS lab_core)

target_include_directories(lab_core
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)

# Boost is consumed header-only and arrives from the workspace install
# (BOOST_ROOT): leaf (result/error transport), container_hash (fixed_string
# hashing), pfr (lab::auto_hash reflective field walk), algorithm (string
# replace in result.hpp's leaf diagnostic formatter), and stacktrace
# (LAB_ASSERT, header-only mode -- raw addresses, no symbol resolution).
#
# fmt comes from FetchContent (see vendor/CMakeLists.txt) and is consumed
# header-only so spdlog and the lab vocabulary share one fmt ABI.
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
