#pragma once

/// @file userver/testsuite/testpoint.hpp
/// @brief @copybrief TESTPOINT

#include <string>

#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/testsuite/testpoint_control.hpp>

// TODO remove extra includes
#include <fmt/format.h>
#include <atomic>
#include <chrono>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace testsuite::impl {

class TestpointScope final {
 public:
  TestpointScope();
  ~TestpointScope();

  explicit operator bool() const noexcept;

  // The returned client must only be used within Scope's lifetime
  const TestpointClientBase& GetClient() const noexcept;

 private:
  struct Impl;
  utils::FastPimpl<Impl, 24, 8> impl_;
};

bool IsTestpointEnabled(std::string_view name);

void ExecuteTestpointBlocking(const std::string& name,
                              const formats::json::Value& json,
                              const TestpointClientBase::Callback& callback,
                              engine::TaskProcessor& task_processor);

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
/// @hideinitializer

// clang-format on
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TESTPOINT_CALLBACK(name, json, callback)        \
  do {                                                  \
    namespace tp = USERVER_NAMESPACE::testsuite::impl;  \
    if (!tp::IsTestpointEnabled(name)) break;           \
    tp::TestpointScope tp_scope;                        \
    if (!tp_scope) break;                               \
    /* only evaluate 'json' if actually needed */       \
    tp_scope.GetClient().Execute(name, json, callback); \
  } while (false)

// clang-format off

/// @brief Send testpoint notification. Works only if testpoint support is
/// enabled (e.g. in components::TestsControl), otherwise does nothing.
///
/// Example usage:
/// @snippet samples/testsuite-support/src/testpoint.cpp Testpoint - TESTPOINT()
/// @snippet samples/testsuite-support/tests/test_testpoint.py Testpoint - fixture
///
/// @hideinitializer

// clang-format on
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TESTPOINT(name, json) TESTPOINT_CALLBACK(name, json, {})

/// @brief Same as `TESTPOINT_CALLBACK` but must be called outside of
/// coroutine (e.g. from std::thread routine).
///
/// @hideinitializer
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TESTPOINT_CALLBACK_NONCORO(name, json, task_processor, callback) \
  do {                                                                   \
    namespace tp = USERVER_NAMESPACE::testsuite::impl;                   \
    if (!tp::IsTestpointEnabled(name)) break;                            \
    tp::ExecuteTestpointBlocking(name, json, callback, task_processor);  \
  } while (false)

/// @brief Same as `TESTPOINT` but must be called outside of
/// coroutine (e.g. from std::thread routine).
///
/// @hideinitializer
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TESTPOINT_NONCORO(name, json, task_processor) \
  TESTPOINT_CALLBACK_NONCORO(name, json, task_processor, {})
