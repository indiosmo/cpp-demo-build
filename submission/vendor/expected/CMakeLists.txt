if(NOT TARGET kraken_vendor_expected)
  add_library(kraken_vendor_expected INTERFACE)
  add_library(kraken::vendor::expected ALIAS kraken_vendor_expected)
  # PATHS.txt narrows the sparse-checkout to include/tl/expected.hpp and
  # COPYING, so consumers include <tl/expected.hpp> with upstream/include on
  # the path.
  target_include_directories(kraken_vendor_expected SYSTEM INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream/include")
endif()
