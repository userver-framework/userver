#pragma once

/// @file userver/utest/death_tests.hpp
/// @brief Contains facilities for testing service crashes

#include <gtest/gtest.h>

#include <userver/utils/assert.hpp>
#include <userver/utils/impl/disable_core_dumps.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest::impl {

class DeathTestScope final {
 public:
  DeathTestScope() { utils::impl::dump_stacktrace_on_assert_failure = false; }
  ~DeathTestScope() { utils::impl::dump_stacktrace_on_assert_failure = true; }

  bool ShouldKeepIterating() const noexcept { return keep_iterating_; }
  void StopIterating() noexcept { keep_iterating_ = false; }

 private:
  utils::impl::DisableCoreDumps disable_core_dumps_;
  bool keep_iterating_{true};
};

}  // namespace utest::impl

USERVER_NAMESPACE_END

/// @ingroup userver_utest
///
/// @brief An optimized equivalent of EXPECT_DEATH.
///
/// @hideinitializer
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define UEXPECT_DEATH(statement, message)                 \
  for (USERVER_NAMESPACE::utest::impl::DeathTestScope     \
           utest_impl_death_test_scope;                   \
       utest_impl_death_test_scope.ShouldKeepIterating(); \
       utest_impl_death_test_scope.StopIterating())       \
  EXPECT_DEATH(statement, message)
