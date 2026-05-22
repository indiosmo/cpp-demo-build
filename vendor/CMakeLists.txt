# Vendored third-party header-only utilities. Each subdirectory wraps an
# upstream library as an INTERFACE target re-exported under
# lab::vendor::<name>. Production code should depend on the lab::
# alias under src/lab/, not on these targets directly --
# see DEPENDENCIES.md and ADR 0002.
#
# To add or update a dependency, edit its ORIGIN.txt / VERSION.txt /
# PATHS.txt files and run scripts/vendor.sh sync.

add_subdirectory(NamedType)
add_subdirectory(inplace_function)
add_subdirectory(readerwriterqueue)
