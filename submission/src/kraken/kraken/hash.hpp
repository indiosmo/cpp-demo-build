#ifndef KRAKEN_HASH_HPP
#define KRAKEN_HASH_HPP

#include "boost/container_hash/hash.hpp"
#include "boost/pfr.hpp"

#include <cstddef>

namespace kraken {

/*
 * Reflective hasher for any aggregate. boost::pfr walks the fields without
 * macros; boost::hash_combine mixes each field. Saves hand-rolling
 * hash_combine recipes for small key types.
 */
template <typename T>
constexpr std::size_t auto_hash(const T& value)
{
  std::size_t seed = 0;
  boost::pfr::for_each_field(value, [&seed](const auto& field) { boost::hash_combine(seed, field); });
  return seed;
}

} // namespace kraken

/*
 * Specializes std::hash<TYPE> via kraken::auto_hash. Invoke at namespace
 * scope so the specialization lands in ::std.
 */
// clang-format off
#define KRAKEN_STD_HASH(TYPE)                       \
namespace std {                                     \
  template <>                                       \
  struct hash<TYPE>                                 \
  {                                                 \
    std::size_t operator()(const TYPE& value) const \
    {                                               \
      return ::kraken::auto_hash(value);            \
    }                                               \
  };                                                \
}
// clang-format on

/* Hooks the type into boost::hash<T> via ADL hash_value. */
#define KRAKEN_HASH_VALUE(TYPE)                    \
  inline std::size_t hash_value(const TYPE& value) \
  {                                                \
    return ::kraken::auto_hash(value);             \
  }

#endif /* KRAKEN_HASH_HPP */
