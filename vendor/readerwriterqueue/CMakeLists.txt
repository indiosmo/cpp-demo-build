if(NOT TARGET lab_vendor_readerwriterqueue)
  add_library(lab_vendor_readerwriterqueue INTERFACE)
  add_library(lab::vendor::readerwriterqueue ALIAS lab_vendor_readerwriterqueue)
  # upstream layout: headers sit at the repo root (readerwriterqueue.h,
  # atomicops.h, readerwritercircularbuffer.h).
  target_include_directories(lab_vendor_readerwriterqueue SYSTEM INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream")
endif()
