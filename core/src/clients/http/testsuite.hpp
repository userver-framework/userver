#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

namespace clients::http {

struct TestsuiteConfig {
  std::vector<std::string> allowed_url_prefixes;
  std::optional<std::chrono::milliseconds> http_request_timeout;
};

}  // namespace clients::http
