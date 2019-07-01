#pragma once

#include <atomic>
#include <chrono>
#include <string>

#include <formats/json/value.hpp>
#include <utils/assert.hpp>

namespace clients {
namespace http {
class Client;
}
}  // namespace clients

namespace utils {

// Don't use TestPoint directly, use TESTPOINT(name, json) instead
class TestPoint final {
 public:
  static TestPoint& GetInstance();

  // Should be called from server::handlers::TestsControl only
  void Setup(clients::http::Client& http_client, const std::string& url,
             std::chrono::milliseconds timeout);

  void Notify(const std::string& name, const formats::json::Value& json);

  bool IsEnabled() const;

 private:
  std::atomic<bool> is_initialized_{false};
  clients::http::Client* http_client_;
  std::string url_;
  std::chrono::milliseconds timeout_;
};

/// Send testpoint notification if testpoint support is enabled
/// (e.g. in testsuite), otherwise does nothing.
/// See https://wiki.yandex-team.ru/taxi/backend/testsuite/#testpoint for more
/// info.
#define TESTPOINT(name, json)                     \
  do {                                            \
    auto& tp = ::utils::TestPoint::GetInstance(); \
    if (!tp.IsEnabled()) break;                   \
                                                  \
    tp.Notify(name, json);                        \
  } while (0)

}  // namespace utils
