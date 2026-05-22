if(NOT TARGET lab_vendor_NamedType)
  add_library(lab_vendor_NamedType INTERFACE)
  add_library(lab::vendor::NamedType ALIAS lab_vendor_NamedType)
  # upstream layout: include/NamedType/*.hpp; adjust if the upstream tree
  # changes (see DEPENDENCIES.md "Caveats").
  target_include_directories(lab_vendor_NamedType SYSTEM INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream/include")
endif()
