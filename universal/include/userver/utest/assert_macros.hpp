#pragma once

/// @file userver/utest/assert_macros.hpp
/// @brief Extensions to the gtest macros for printing and testing exceptions
/// that could work even without coroutine environment.
/// @ingroup userver_universal

#include <exception>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>

#include <gtest/gtest.h>

#include <userver/utest/death_tests.hpp>
#include <userver/utest/impl/assert_macros.hpp>
#include <userver/utils/invariant_error.hpp>

/// @cond
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define IMPL_UTEST_ASSERT_THROW(statement, exception_type, message_substring, \
                                failure_macro)                                \
  if (const auto message_impl_utest =                                         \
          USERVER_NAMESPACE::utest::impl::AssertThrow(                        \
              [&] { statement; }, #statement,                                 \
              &USERVER_NAMESPACE::utest::impl::IsSubtype<exception_type>,     \
              typeid(exception_type), message_substring);                     \
      !message_impl_utest.empty())                                            \
  failure_macro(message_impl_utest.c_str())

// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define IMPL_UTEST_ASSERT_NO_THROW(statement, failure_macro)                \
  if (const auto message_impl_utest =                                       \
          USERVER_NAMESPACE::utest::impl::AssertNoThrow([&] { statement; }, \
                                                        #statement);        \
      !message_impl_utest.empty())                                          \
  failure_macro(message_impl_utest.c_str())
/// @endcond

/// @ingroup userver_utest
///
/// An equivalent to `EXPECT_THROW` with an additional check for a message
/// substring
///
/// @hideinitializer
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define UEXPECT_THROW_MSG(statement, exception_type, message_substring) \
  IMPL_UTEST_ASSERT_THROW(statement, exception_type, message_substring, \
                          GTEST_NONFATAL_FAILURE_)

/// @ingroup userver_utest
///
/// An equivalent to `ASSERT_THROW` with an additional check for a message
/// substring
///
/// @hideinitializer
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define UASSERT_THROW_MSG(statement, exception_type, message_substring) \
  IMPL_UTEST_ASSERT_THROW(statement, exception_type, message_substring, \
                          GTEST_FATAL_FAILURE_)

/// @ingroup userver_utest
///
/// An equivalent to `EXPECT_THROW` with better diagnostics
///
/// @hideinitializer
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define UEXPECT_THROW(statement, exception_type)         \
  IMPL_UTEST_ASSERT_THROW(statement, exception_type, "", \
                          GTEST_NONFATAL_FAILURE_)

/// @ingroup userver_utest
///
/// An equivalent to `ASSERT_THROW` with better diagnostics
///
/// @hideinitializer
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define UASSERT_THROW(statement, exception_type) \
  IMPL_UTEST_ASSERT_THROW(statement, exception_type, "", GTEST_FATAL_FAILURE_)

/// @ingroup userver_utest
///
/// An equivalent to `EXPECT_NO_THROW` with better diagnostics
///
/// @hideinitializer
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define UEXPECT_NO_THROW(statement) \
  IMPL_UTEST_ASSERT_NO_THROW(statement, GTEST_NONFATAL_FAILURE_)

/// @ingroup userver_utest
///
/// An equivalent to `EXPECT_THROW` with better diagnostics
///
/// @hideinitializer
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define UASSERT_NO_THROW(statement) \
  IMPL_UTEST_ASSERT_NO_THROW(statement, GTEST_FATAL_FAILURE_)

/// @cond
#ifdef NDEBUG
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define EXPECT_UINVARIANT_FAILURE_MSG(statement, message_substring)      \
  UEXPECT_THROW_MSG(statement, USERVER_NAMESPACE::utils::InvariantError, \
                    message_substring)
#else
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define EXPECT_UINVARIANT_FAILURE_MSG(statement, message_substring) \
  UEXPECT_DEATH(statement, message_substring)
#endif
/// @endcond

/// @ingroup userver_utest
///
/// Test that a UINVARIANT check triggers
///
/// @hideinitializer
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define EXPECT_UINVARIANT_FAILURE(statement) \
  EXPECT_UINVARIANT_FAILURE_MSG(statement, "")
