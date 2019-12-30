#pragma once

/// @file testsuite/testpoint.hpp
/// @brief @copybrief TESTPOINT

#include <atomic>
#include <chrono>
#include <string>

#include <formats/json/value.hpp>
#include <utils/assert.hpp>

namespace clients::http {
class Client;
}  // namespace clients::http

namespace testsuite::impl {

// Don't use TestPoint directly, use TESTPOINT(name, json) instead
class TestPoint final {
 public:
  static TestPoint& GetInstance();

  // Should be called from server::handlers::TestsControl only
  void Setup(clients::http::Client& http_client, const std::string& url,
             std::chrono::milliseconds timeout);

  void Notify(
      const std::string& name, const formats::json::Value& json,
      const std::function<void(const formats::json::Value&)>& callback = {});

  bool IsEnabled() const;

 private:
  std::atomic<bool> is_initialized_{false};
  clients::http::Client* http_client_;
  std::string url_;
  std::chrono::milliseconds timeout_;
};

}  // namespace testsuite::impl

/// @brief Send testpoint notification if testpoint support is enabled
/// (e.g. in testsuite), otherwise does nothing.
/// @see https://wiki.yandex-team.ru/taxi/backend/testsuite/#testpoint
#define TESTPOINT(name, json) TESTPOINT_CALLBACK(name, json, {})

#define TESTPOINT_CALLBACK(name, json, callback)            \
  do {                                                      \
    auto& tp = ::testsuite::impl::TestPoint::GetInstance(); \
    if (!tp.IsEnabled()) break;                             \
                                                            \
    tp.Notify(name, json, callback);                        \
  } while (0)
