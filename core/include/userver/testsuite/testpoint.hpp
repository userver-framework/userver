#pragma once

/// @file userver/testsuite/testpoint.hpp
/// @brief @copybrief TESTPOINT

#include <string>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utils/function_ref.hpp>

// TODO remove extra includes
#include <fmt/format.h>
#include <atomic>
#include <chrono>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite {

/// @brief Returns true if testpoints are available in runtime.
bool AreTestpointsAvailable() noexcept;

using TestpointCallback =
    utils::function_ref<void(const formats::json::Value&)>;

}  // namespace testsuite

namespace testsuite::impl {

bool IsTestpointEnabled(std::string_view name) noexcept;

void ExecuteTestpointCoro(std::string_view name,
                          const formats::json::Value& json,
                          TestpointCallback callback);

void ExecuteTestpointBlocking(std::string_view name,
                              const formats::json::Value& json,
                              TestpointCallback callback,
                              engine::TaskProcessor& task_processor);

void DoNothing(const formats::json::Value&) noexcept;

}  // namespace testsuite::impl

USERVER_NAMESPACE_END

// clang-format off

/// @brief Send testpoint notification and receive data. Works only if
/// testpoint support is enabled (e.g. in components::TestsControl),
/// otherwise does nothing.
///
/// Example usage:
/// @snippet samples/testsuite-support/src/testpoint.cpp Sample TESTPOINT_CALLBACK usage cpp
/// @snippet samples/testsuite-support/tests/test_testpoint.py Sample TESTPOINT_CALLBACK usage python
///
/// Throws nothing if server::handlers::TestsControl is not
/// loaded or it is disabled in static config via `load-enabled: false`.
///
/// @hideinitializer

// clang-format on
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TESTPOINT_CALLBACK(name, json, callback)                            \
  do {                                                                      \
    if (__builtin_expect(                                                   \
            !USERVER_NAMESPACE::testsuite::AreTestpointsAvailable(), true)) \
      break;                                                                \
                                                                            \
    /* cold testing path: */                                                \
    const auto& userver_impl_tp_name = name;                                \
    if (!USERVER_NAMESPACE::testsuite::impl::IsTestpointEnabled(            \
            userver_impl_tp_name))                                          \
      break;                                                                \
                                                                            \
    USERVER_NAMESPACE::testsuite::impl::ExecuteTestpointCoro(               \
        userver_impl_tp_name, json, callback);                              \
  } while (false)

// clang-format off

/// @brief Send testpoint notification. Works only if testpoint support is
/// enabled (e.g. in components::TestsControl), otherwise does nothing.
///
/// Example usage:
/// @snippet samples/testsuite-support/src/testpoint.cpp Testpoint - TESTPOINT()
/// @snippet samples/testsuite-support/tests/test_testpoint.py Testpoint - fixture
///
/// Throws nothing if server::handlers::TestsControl is not
/// loaded or it is disabled in static config via `load-enabled: false`.
///
/// @hideinitializer

// clang-format on
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TESTPOINT(name, json) \
  TESTPOINT_CALLBACK(name, json, USERVER_NAMESPACE::testsuite::impl::DoNothing)

/// @brief Same as `TESTPOINT_CALLBACK` but must be called outside of
/// coroutine (e.g. from std::thread routine).
///
/// Throws nothing if server::handlers::TestsControl is not
/// loaded or it is disabled in static config via `load-enabled: false`.
///
/// @hideinitializer
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TESTPOINT_CALLBACK_NONCORO(name, json, task_processor, callback)    \
  do {                                                                      \
    if (__builtin_expect(                                                   \
            !USERVER_NAMESPACE::testsuite::AreTestpointsAvailable(), true)) \
      break;                                                                \
                                                                            \
    /* cold testing path: */                                                \
    const auto& userver_impl_tp_name = name;                                \
    if (!USERVER_NAMESPACE::testsuite::impl::IsTestpointEnabled(            \
            userver_impl_tp_name))                                          \
      break;                                                                \
                                                                            \
    USERVER_NAMESPACE::testsuite::impl::ExecuteTestpointBlocking(           \
        userver_impl_tp_name, json, callback, task_processor);              \
  } while (false)

/// @brief Same as `TESTPOINT` but must be called outside of
/// coroutine (e.g. from std::thread routine).
///
/// Throws nothing if server::handlers::TestsControl is not
/// loaded or it is disabled in static config via `load-enabled: false`.
///
/// @hideinitializer
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TESTPOINT_NONCORO(name, json, task_processor)    \
  TESTPOINT_CALLBACK_NONCORO(name, json, task_processor, \
                             USERVER_NAMESPACE::testsuite::impl::DoNothing)
