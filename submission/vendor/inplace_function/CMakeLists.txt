if(NOT TARGET kraken_vendor_inplace_function)
  add_library(kraken_vendor_inplace_function INTERFACE)
  add_library(kraken::vendor::inplace_function ALIAS kraken_vendor_inplace_function)
  # PATHS.txt narrows the sparse-checkout to SG14/inplace_function.h, so the
  # consumer #includes <SG14/inplace_function.h> with upstream/ on the path.
  target_include_directories(kraken_vendor_inplace_function SYSTEM INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream")
endif()
