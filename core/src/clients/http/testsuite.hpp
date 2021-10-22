#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

struct TestsuiteConfig {
  std::vector<std::string> allowed_url_prefixes;
  std::optional<std::chrono::milliseconds> http_request_timeout;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
