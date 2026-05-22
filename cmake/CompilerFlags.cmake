# Compiler flags for matching-engine-lab.
#
# Provides the lab::compiler_flags INTERFACE library which carries the
# warnings and sanitizer options used across the project. Targets are linked
# against it automatically via link_libraries() below.

set(CMAKE_DEBUG_POSTFIX d)

set(GCC_WARNINGS
  # Common warnings about questionable constructions
  -Wall

  # Additional warnings not covered by -Wall
  -Wextra

  # Non-compliance with strict ISO C/C++
  -Wpedantic

  # Turn all warnings into errors
  -Werror

  # Make pedantic warnings into errors
  -pedantic-errors

  # Paths that dereference null pointers
  -Wnull-dereference

  # Dangling references
  -Wdangling-reference

  # Duplicated conditions in if-else chains
  -Wduplicated-cond

  # If-else has identical branches
  -Wduplicated-branches

  # Static functions that are never used
  -Wunused-function

  # Suspicious logical operations
  -Wlogical-op

  # Restrict qualifier violations
  -Wrestrict

  # Pointer casts that increase alignment requirements
  -Wcast-align=strict

  # Implicit type conversions that may change value
  -Wconversion

  # Using uninitialized automatic variables
  -Wuninitialized

  # All uses of alloca
  -Walloca

  # Casts that remove type qualifiers
  -Wcast-qual

  # Class only has private constructors/destructors
  -Wctor-dtor-privacy

  # Deprecated implicit copy operations
  -Wdeprecated-copy-dtor

  # Float implicitly promoted to double
  -Wdouble-promotion

  # Implicit conversions between different enum types
  -Wenum-conversion

  # Buffer overflows in formatted I/O functions
  -Wformat-overflow=2

  # Format string signedness mismatches
  -Wformat-signedness

  # Check printf/scanf format strings and arguments
  -Wformat=2

  # Multicharacter constants
  -Wmultichar

  # Non-virtual destructors in base classes
  -Wnon-virtual-dtor

  # Pointer arithmetic on void/function pointers
  -Wpointer-arith

  # Inefficient range-based for loop constructs
  -Wrange-loop-construct

  # Missing null sentinel in variadic functions
  -Wstrict-null-sentinel

  # Suggest adding format attribute to functions
  -Wsuggest-attribute=format

  # Variable length arrays
  -Wvla

  # Deprecated volatile usage patterns
  -Wvolatile

  # String literals without const qualifier
  -Wwrite-strings

  # Undefined macros in #if directives
  -Wundef

  # Warn on switch case fallthrough without explicit annotation
  -Wimplicit-fallthrough

  # Extra semicolons after member function definitions
  -Wextra-semi

  # GCC warns that std::atomic_thread_fence is not modeled by ThreadSanitizer.
  # Boost.Asio and readerwriterqueue both use it internally, so keep TSan
  # instrumentation enabled but do not promote that third-party limitation to a
  # build failure.
  $<$<BOOL:${LAB_TSAN}>:-Wno-tsan>
)

set(GCC_OPTIONS
  # Emit debug information tailored for GDB
  -ggdb

  # Always colorize compiler diagnostics
  -fdiagnostics-color=always

  # Preserve frame pointers so profilers and sanitizers get accurate stack traces
  -fno-omit-frame-pointer

  $<$<BOOL:${LAB_RELEASE}>:-O2>
  
  # Address Sanitizer flags
  $<$<BOOL:${LAB_ASAN}>:-fsanitize=address>
  $<$<BOOL:${LAB_ASAN}>:-fsanitize=leak>
  $<$<BOOL:${LAB_ASAN}>:-fsanitize=undefined>
  $<$<BOOL:${LAB_ASAN}>:-fsanitize-address-use-after-scope>

  # ThreadSanitizer flags
  $<$<BOOL:${LAB_TSAN}>:-fsanitize=thread>
)

set(CLANG_WARNINGS
  # Common warnings about questionable constructions
  -Wall

  # Additional warnings not covered by -Wall
  -Wextra

  # Non-compliance with strict ISO C/C++
  -Wpedantic

  # Turn all warnings into errors
  -Werror

  # Code that can never be executed
  -Wunreachable-code

  # Using uninitialized automatic variables
  -Wuninitialized

  # Non-virtual destructors in base classes
  -Wnon-virtual-dtor

  # Function overrides a virtual function but does not override all overloads
  -Woverloaded-virtual

  # Paths that dereference null pointers
  -Wnull-dereference

  # Suspicious uses of the comma operator
  -Wcomma

  # Pointer casts that increase alignment requirements
  -Wcast-align

  # Casts that remove type qualifiers
  -Wcast-qual

  # Implicit type conversions that may change value
  -Wconversion

  # Float implicitly promoted to double
  -Wdouble-promotion

  # Format string security issues
  -Wformat-security

  # Check printf/scanf format strings and arguments
  -Wformat=2

  # Pointer arithmetic on void/function pointers
  -Wpointer-arith

  # Left shift overflow
  -Wshift-overflow

  # String literals without const qualifier
  -Wwrite-strings

  # Undefined macros in #if directives
  -Wundef

  # Variable length arrays
  -Wvla

  # Redundant declarations of the same entity
  -Wredundant-decls

  # Global function defined without a prior declaration
  -Wmissing-declarations

  # Self-comparisons that are always true or false
  -Wtautological-compare

  # Macros defined but never used
  -Wunused-macros
)

set(CLANG_COMPILE_OPTIONS
  -ggdb
  -fno-omit-frame-pointer

  $<$<BOOL:${LAB_RELEASE}>:-O2>

  $<$<BOOL:${LAB_ASAN}>:-fsanitize=address>
  $<$<BOOL:${LAB_ASAN}>:-fsanitize=leak>
  $<$<BOOL:${LAB_ASAN}>:-fsanitize=undefined>
  $<$<BOOL:${LAB_ASAN}>:-fsanitize-address-use-after-scope>

  $<$<BOOL:${LAB_TSAN}>:-fsanitize=thread>
)

add_library(lab_compiler_flags INTERFACE)
add_library(lab::compiler_flags ALIAS lab_compiler_flags)

target_compile_options(lab_compiler_flags BEFORE INTERFACE
  $<$<CXX_COMPILER_ID:GNU>:${GCC_OPTIONS}>
  $<$<CXX_COMPILER_ID:GNU>:${GCC_WARNINGS}>
  $<$<CXX_COMPILER_ID:Clang>:${CLANG_COMPILE_OPTIONS}>
  $<$<CXX_COMPILER_ID:Clang>:${CLANG_WARNINGS}>
)

target_compile_definitions(lab_compiler_flags INTERFACE
  $<$<BOOL:${LAB_DEBUG}>:_GLIBCXX_ASSERTIONS>
)

target_link_libraries(lab_compiler_flags INTERFACE
  $<$<BOOL:${LAB_ASAN}>:-fsanitize=address>
  $<$<BOOL:${LAB_ASAN}>:-fsanitize=leak>
  $<$<BOOL:${LAB_ASAN}>:-fsanitize=undefined>
  $<$<BOOL:${LAB_ASAN}>:-fsanitize-address-use-after-scope>
  $<$<BOOL:${LAB_TSAN}>:-fsanitize=thread>
)

# All targets added after this point inherit lab::compiler_flags so the
# warning/sanitizer configuration is applied uniformly across the project.
link_libraries(lab::compiler_flags)
