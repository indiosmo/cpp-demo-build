#ifndef LAB_EXPECTED_HPP
#define LAB_EXPECTED_HPP

#include <expected>
#include <type_traits>
#include <utility>

/*
 * Project vocabulary for value-or-error boundary APIs.
 */

namespace lab {

template <typename T, typename E>
using expected = std::expected<T, E>;

template <typename E>
using unexpected = std::unexpected<E>;

using std::bad_expected_access;
using std::unexpect;
using std::unexpect_t;

template <typename E>
constexpr auto make_unexpected(E&& error)
{
  return std::unexpected<std::decay_t<E>>{std::forward<E>(error)};
}

} // namespace lab

#endif /* LAB_EXPECTED_HPP */
