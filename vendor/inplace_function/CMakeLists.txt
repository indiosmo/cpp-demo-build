if(NOT TARGET lab_vendor_inplace_function)
  add_library(lab_vendor_inplace_function INTERFACE)
  add_library(lab::vendor::inplace_function ALIAS lab_vendor_inplace_function)
  # PATHS.txt narrows the sparse-checkout to SG14/inplace_function.h, so the
  # consumer #includes <SG14/inplace_function.h> with upstream/ on the path.
  target_include_directories(lab_vendor_inplace_function SYSTEM INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream")
endif()
