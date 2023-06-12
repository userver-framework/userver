#pragma once

/// @file userver/utest/assert_macros.hpp
/// @brief Extensions to the gtest macros for printing and testing exceptions

#include <exception>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>

#include <gtest/gtest.h>

#include <userver/utest/death_tests.hpp>
#include <userver/utils/invariant_error.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest::impl {

template <typename ExceptionType>
bool IsSubtype(const std::exception& ex) noexcept {
  static_assert(
      std::is_base_of_v<std::exception, ExceptionType>,
      "Exception types not inherited from std::exception are not supported");
  if constexpr (std::is_same_v<ExceptionType, std::exception>) {
    return true;
  } else {
    return dynamic_cast<const ExceptionType*>(&ex) != nullptr;
  }
}

std::string AssertThrow(std::function<void()> statement,
                        std::string_view statement_text,
                        std::function<bool(const std::exception&)> type_checker,
                        const std::type_info& expected_type,
                        std::string_view message_substring);

std::string AssertNoThrow(std::function<void()> statement,
                          std::string_view statement_text);

}  // namespace utest::impl

USERVER_NAMESPACE_END

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

/// @ingroup userver_utest
///
/// Test that a UINVARIANT check triggers
///
/// @hideinitializer
#ifdef NDEBUG
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define EXPECT_UINVARIANT_FAILURE(statement) \
  UEXPECT_THROW(statement, USERVER_NAMESPACE::utils::InvariantError)
#else
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define EXPECT_UINVARIANT_FAILURE(statement) UEXPECT_DEATH(statement, "")
#endif
