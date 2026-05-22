if(NOT TARGET kraken_vendor_readerwriterqueue)
  add_library(kraken_vendor_readerwriterqueue INTERFACE)
  add_library(kraken::vendor::readerwriterqueue ALIAS kraken_vendor_readerwriterqueue)
  # upstream layout: headers sit at the repo root (readerwriterqueue.h,
  # atomicops.h, readerwritercircularbuffer.h).
  target_include_directories(kraken_vendor_readerwriterqueue SYSTEM INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream")
endif()
