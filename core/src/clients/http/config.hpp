#pragma once

#include <chrono>
#include <string>

#include <clients/http/enforce_task_deadline_config.hpp>
#include <userver/dynamic_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

struct Config {
  static constexpr size_t kNoLimit = -1UL;
  static constexpr size_t kDefaultConnectionPoolSize = 10000;

  Config() = default;
  explicit Config(const dynamic_config::DocsMap& docs_map);

  std::size_t connection_pool_size{kDefaultConnectionPoolSize};
  EnforceTaskDeadlineConfig enforce_task_deadline;

  size_t http_connect_throttle_limit{kNoLimit};
  std::chrono::microseconds http_connect_throttle_rate{0};
  size_t https_connect_throttle_limit{kNoLimit};
  std::chrono::microseconds https_connect_throttle_rate{0};
  size_t per_host_connect_throttle_limit{kNoLimit};
  std::chrono::microseconds per_host_connect_throttle_rate{0};

  std::string proxy;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
