#ifndef KRAKEN_EXPECTED_HPP
#define KRAKEN_EXPECTED_HPP

#include "tl/expected.hpp"

/*
 * Aliased from tl::expected (C++20 backport of std::expected) so the alias
 * collapses to std::expected when the project moves to C++23.
 */

namespace kraken {

template <typename T, typename E>
using expected = tl::expected<T, E>;

template <typename E>
using unexpected = tl::unexpected<E>;

using tl::bad_expected_access;
using tl::make_unexpected;
using tl::unexpect;
using tl::unexpect_t;

} // namespace kraken

#endif /* KRAKEN_EXPECTED_HPP */
