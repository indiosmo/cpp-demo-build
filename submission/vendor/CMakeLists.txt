# Vendored third-party header-only utilities. Each subdirectory wraps an
# upstream library as an INTERFACE target re-exported under
# kraken::vendor::<name>. Production code should depend on the kraken::
# alias under submission/src/kraken/, not on these targets directly --
# see DEPENDENCIES.md and ADR 0002.
#
# To add or update a dependency, edit its ORIGIN.txt / VERSION.txt /
# PATHS.txt files and run scripts/vendor.sh sync.

add_subdirectory(NamedType)
add_subdirectory(inplace_function)
add_subdirectory(readerwriterqueue)
