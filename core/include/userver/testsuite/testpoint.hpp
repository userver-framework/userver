#pragma once

/// @file userver/testsuite/testpoint.hpp
/// @brief @copybrief TESTPOINT

#include <atomic>
#include <chrono>
#include <future>
#include <string>
#include <type_traits>
#include <unordered_set>

#include <fmt/format.h>

#include <userver/formats/json/value.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class Client;
}  // namespace clients::http

namespace testsuite::impl {

/// @brief Don't use TestPoint directly, use TESTPOINT(name, json) instead
/// @note All methods are thread-safe after initialization by `Setup`
class TestPoint final {
 public:
  static TestPoint& GetInstance();

  // Must only be called from server::handlers::TestsControl at startup
  void Setup(clients::http::Client& http_client, const std::string& url,
             std::chrono::milliseconds timeout,
             bool skip_unregistered_testpoints);

  void Notify(
      const std::string& name, const formats::json::Value& json,
      const std::function<void(const formats::json::Value&)>& callback = {});

  void RegisterPaths(std::unordered_set<std::string> paths);

  bool IsRegisteredPath(const std::string& path) const;

  bool IsEnabled() const;

 private:
  std::atomic<bool> is_initialized_{false};
  clients::http::Client* http_client_{nullptr};
  std::string url_;
  std::chrono::milliseconds timeout_{};
  rcu::Variable<std::unordered_set<std::string>> registered_paths_;
  bool skip_unregistered_testpoints_{false};
};

}  // namespace testsuite::impl

/// @brief Send testpoint notification if testpoint support is enabled
/// (e.g. in components::TestsuiteSupport), otherwise does nothing.
///
/// Example usage:
/// @snippet testsuite/testpoint_test.cpp Sample TESTPOINT_CALLBACK usage cpp
/// @snippet testsuite/testpoint_test.cpp Sample TESTPOINT_CALLBACK usage python
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TESTPOINT(name, json) TESTPOINT_CALLBACK(name, json, {})

/// @brief Send testpoint notification and receive data. Works only if
/// testpoint support is enabled (e.g. in components::TestsuiteSupport),
/// otherwise does nothing.
///
/// Example usage:
/// @snippet testsuite/testpoint_test.cpp Sample TESTPOINT_CALLBACK usage cpp
/// @snippet testsuite/testpoint_test.cpp Sample TESTPOINT_CALLBACK usage python
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TESTPOINT_CALLBACK(name, json, callback)                             \
  do {                                                                       \
    auto& tp = USERVER_NAMESPACE::testsuite::impl::TestPoint::GetInstance(); \
    if (!tp.IsEnabled()) break;                                              \
    if (!tp.IsRegisteredPath(name)) break;                                   \
                                                                             \
    tp.Notify(name, json, callback);                                         \
  } while (0)

/// @brief Same as `TESTPOINT_CALLBACK` but must be called outside of
/// coroutine (e.g. from std::thread routine).
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TESTPOINT_CALLBACK_NONCORO(name, json, task_p, callback)               \
  do {                                                                         \
    auto& tp = USERVER_NAMESPACE::testsuite::impl::TestPoint::GetInstance();   \
    if (!tp.IsEnabled()) break;                                                \
    if (!tp.IsRegisteredPath(name)) break;                                     \
                                                                               \
    auto j = (json);                                                           \
    auto c = (callback);                                                       \
    auto n = (name);                                                           \
    auto cb = [&n, &j, &c, &tp] { tp.Notify(n, j, c); };                       \
    std::packaged_task<void()> task(cb);                                       \
    auto future = task.get_future();                                           \
    engine::CriticalAsyncNoSpan(task_p, std::move(task)).Detach();             \
    try {                                                                      \
      future.get();                                                            \
    } catch (const std::exception& e) {                                        \
      UASSERT_MSG(false, fmt::format("Unhandled exception from testpoint: {}", \
                                     e.what()));                               \
      throw;                                                                   \
    }                                                                          \
  } while (0)

/// @brief Same as `TESTPOINT` but must be called outside of
/// coroutine (e.g. from std::thread routine).
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TESTPOINT_NONCORO(name, j, task_p)    \
  TESTPOINT_CALLBACK_NONCORO(name, j, task_p, \
                             ([](const formats::json::Value&) {}))

USERVER_NAMESPACE_END
