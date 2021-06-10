#pragma once

#include <chrono>
#include <functional>
#include <utility>

#include <gtest/gtest.h>

#include <engine/run_in_coro.hpp>  // legacy
#include <utest/test_case_macros.hpp>
#include <utils/assert.hpp>
#include <utils/strong_typedef.hpp>

/// Deprecated, use `UTEST_X` macros instead
void TestInCoro(std::function<void()> callback, std::size_t worker_threads = 1);

inline constexpr std::chrono::seconds kMaxTestWaitTime(20);

// gtest-specific serializers
namespace testing {

void PrintTo(std::chrono::seconds s, std::ostream* os);
void PrintTo(std::chrono::milliseconds ms, std::ostream* os);
void PrintTo(std::chrono::microseconds us, std::ostream* os);
void PrintTo(std::chrono::nanoseconds ns, std::ostream* os);

}  // namespace testing

namespace formats::json {

class Value;

void PrintTo(const Value&, std::ostream*);

}  // namespace formats::json

namespace utils {

template <class Tag, class T, StrongTypedefOps Ops>
void PrintTo(const StrongTypedef<Tag, T, Ops>& v, std::ostream* os) {
  ::testing::internal::UniversalTersePrint(v.GetUnderlying(), os);
}

}  // namespace utils

namespace decimal64 {

template <int Prec, typename RoundPolicy>
class Decimal;

template <int Prec, typename RoundPolicy>
void PrintTo(const Decimal<Prec, RoundPolicy>& v, std::ostream* os) {
  *os << v;
}

}  // namespace decimal64

#ifdef __APPLE__
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISABLED_IN_MAC_OS_TEST_NAME(name) DISABLED_##name
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISABLED_IN_MAC_OS_TEST_NAME(name) name
#endif

#ifdef _LIBCPP_VERSION
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISABLED_IN_LIBCPP_TEST_NAME(name) DISABLED_##name
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISABLED_IN_LIBCPP_TEST_NAME(name) name
#endif

/// @{
/// Test that a YTX_INVARIANT check triggers
///
/// @hideinitializer
#ifdef NDEBUG
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define EXPECT_YTX_INVARIANT_FAILURE(statement) \
  EXPECT_THROW(statement, ::utils::InvariantError)
#else
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define EXPECT_YTX_INVARIANT_FAILURE(statement) EXPECT_DEATH(statement, "")
#endif
/// @}

namespace utest {

struct Threads final {
  explicit Threads(std::size_t worker_threads) : value(worker_threads) {}

  std::size_t value;
};

}  // namespace utest

/// @{
/// @brief Versions of gtest macros that run tests in a coroutine environment
///
/// There are the following extensions:
///
/// 1. `utest::Threads` can be passed as the 3rd parameter at the test
///    definition. It specifies the worker thread count used in the test.
///    By default, there is only 1 worker thread, which should be enough
///    for most tests.
/// 2. `GetThreadCount()` method is available in the test scope
///
/// ## Usage examples:
/// @snippet core/src/engine/semaphore_test.cpp  UTEST macro example 1
/// @snippet core/src/engine/semaphore_test.cpp  UTEST macro example 2
///
/// @hideinitializer
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define UTEST(test_suite_name, test_name, ...)                        \
  IMPL_UTEST_GTEST_TEST_(test_suite_name, test_name, ::testing::Test, \
                         ::testing::internal::GetTestTypeId(),        \
                         ::utest::impl::CoroutineTestWrapper,         \
                         ::utest::impl::GetThreadCount(__VA_ARGS__))

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define UTEST_F(test_suite_name, test_name, ...)                            \
  IMPL_UTEST_GTEST_TEST_(test_suite_name, test_name, test_suite_name,       \
                         ::testing::internal::GetTypeId<test_suite_name>(), \
                         ::utest::impl::CoroutineTestWrapper,               \
                         ::utest::impl::GetThreadCount(__VA_ARGS__))

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define UTEST_P(test_suite_name, test_name, ...)                   \
  IMPL_UTEST_TEST_P(test_suite_name, test_name,                    \
                    ::utest::impl::CoroutineTestWrapperParametric, \
                    ::utest::impl::GetThreadCount(__VA_ARGS__))

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TYPED_UTEST(test_suite_name, test_name, ...)         \
  IMPL_UTEST_TYPED_TEST(test_suite_name, test_name,          \
                        ::utest::impl::CoroutineTestWrapper, \
                        ::utest::impl::GetThreadCount(__VA_ARGS__))

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TYPED_UTEST_P(test_suite_name, test_name, ...) \
  IMPL_UTEST_TYPED_TEST_P(test_suite_name, test_name,  \
                          ::utest::impl::GetThreadCount(__VA_ARGS__))

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define REGISTER_TYPED_UTEST_SUITE_P(test_suite_name, ...) \
  IMPL_UTEST_REGISTER_TYPED_TEST_SUITE_P(                  \
      test_suite_name, ::utest::impl::CoroutineTestWrapper, __VA_ARGS__)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INSTANTIATE_UTEST_SUITE_P(...) INSTANTIATE_TEST_SUITE_P(__VA_ARGS__)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TYPED_UTEST_SUITE(...) TYPED_TEST_SUITE(__VA_ARGS__)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TYPED_UTEST_SUITE_P(...) TYPED_TEST_SUITE_P(__VA_ARGS__)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INSTANTIATE_TYPED_UTEST_SUITE_P(...) \
  INSTANTIATE_TYPED_TEST_SUITE_P(__VA_ARGS__)
/// @}
