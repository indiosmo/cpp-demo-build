#ifndef LAB_INPLACE_FUNCTION_HPP
#define LAB_INPLACE_FUNCTION_HPP

#include "SG14/inplace_function.h"

/*
 * Type-erased callable with fixed inline storage, aliased from SG14
 * inplace_function. Capture-size is part of the type, so regressions surface
 * as compile errors and the hot path never allocates. Default capacity covers
 * the wiring lambdas (a couple of references plus a small handle); domain
 * code overrides at the declaration site when needed.
 */

namespace lab {

inline constexpr std::size_t default_inplace_function_capacity = 64;

template <typename Signature, std::size_t Capacity = default_inplace_function_capacity>
using inplace_function = stdext::inplace_function<Signature, Capacity>;

} // namespace lab

#endif /* LAB_INPLACE_FUNCTION_HPP */
