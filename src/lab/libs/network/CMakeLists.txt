# Boost.Asio UDP receiver. Treated as a vendor-SDK adapter: the Asio
# headers and the BOOST_ASIO_HEADER_ONLY macro are confined to this
# target so callers see the receiver through its concrete API. Boost
# arrives transitively through lab::core.

add_library(lab_network_asio STATIC
  src/asio_udp_receiver.cpp
)
add_library(lab::network::asio ALIAS lab_network_asio)

target_link_libraries(lab_network_asio
  PUBLIC
    lab::core
)

# Boost.Asio is consumed header-only; BOOST_ASIO_HEADER_ONLY inlines the
# boost::system symbols Asio depends on so the static library has no
# additional link-time dependency on libboost_system.
target_compile_definitions(lab_network_asio
  PUBLIC
    BOOST_ASIO_HEADER_ONLY
)

# Boost.Asio's executor comparison operators inline a pointer compare that
# GCC flags under -Wnull-dereference at -O2. The pointers are valid in
# practice; the warning is a false positive from cross-header inlining. The
# suppression is scoped to the source files that pull in asio so first-party
# code elsewhere is still checked under the project's strict warning policy.
# COMPILE_OPTIONS on the source itself is appended after the target's and
# interface's options so the -Wno- form actually wins.
set_source_files_properties(
  src/asio_udp_receiver.cpp
  PROPERTIES COMPILE_OPTIONS "-Wno-null-dereference"
)

# Solarflare ef_vi kernel-bypass UDP receiver. The stub stays in the tree
# so the layout and CMake split can be exercised without the vendor
# libraries on the host, and so the order_entry runtime composer can
# offer ef_vi as a selectable backend through its receiver_config variant.
# A real integration would gate find_package(EfVi REQUIRED) and the
# Solarflare imported targets behind LAB_NETWORK_ENABLE_REAL_EF_VI,
# keeping the asio consumers free of any vendor coupling when that flag
# is OFF.
add_library(lab_network_ef_vi STATIC
  src/ef_vi_udp_receiver.cpp
)
add_library(lab::network::ef_vi ALIAS lab_network_ef_vi)

target_link_libraries(lab_network_ef_vi
  PUBLIC
    lab::core
)

if(LAB_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()
