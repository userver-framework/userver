#pragma once

namespace clients {
namespace http {

struct TestsuiteConfig {
  std::vector<std::string> allowed_url_prefixes;
  boost::optional<std::chrono::milliseconds> http_request_timeout;
};

}  // namespace http
}  // namespace clients
