if(NOT TARGET kraken_vendor_NamedType)
  add_library(kraken_vendor_NamedType INTERFACE)
  add_library(kraken::vendor::NamedType ALIAS kraken_vendor_NamedType)
  # upstream layout: include/NamedType/*.hpp; adjust if the upstream tree
  # changes (see DEPENDENCIES.md "Caveats").
  target_include_directories(kraken_vendor_NamedType SYSTEM INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream/include")
endif()
