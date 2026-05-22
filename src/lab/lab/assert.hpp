#ifndef LAB_ASSERT_HPP
#define LAB_ASSERT_HPP

#include "boost/stacktrace.hpp"

#include <cstdio>
#include <cstdlib>

/*
 * Runtime assertion macro. Prints the failing expression, source location,
 * and a best-effort stacktrace to stderr, then aborts. Stacktrace frames may
 * appear as raw addresses unless the binary is linked against a symbolising
 * stacktrace backend. NDEBUG elides the abort but keeps the print.
 */

#ifdef NDEBUG
#define LAB_ABORT() (void)0
#else
#define LAB_ABORT() std::abort()
#endif

#define LAB_ASSERT(expr)                                                                             \
  (static_cast<bool>(expr) ? void(0)                                                                    \
                           : (std::fprintf(                                                             \
                                stderr,                                                                 \
                                "assert failed: `%s` at %s:%d\n%s\n",                                   \
                                #expr,                                                                  \
                                __FILE__,                                                               \
                                __LINE__,                                                               \
                                boost::stacktrace::to_string(boost::stacktrace::stacktrace()).c_str()), \
                              LAB_ABORT()))

#endif /* LAB_ASSERT_HPP */
