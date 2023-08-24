#pragma once

#include <chrono>
#include <functional>
#include <string>

#include <userver/components/component_fwd.hpp>
#include <userver/formats/json_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class Client;
}  // namespace clients::http

namespace testsuite {

class HttpAllowedUrlsExtra final {
 public:
  void RegisterHttpClient(clients::http::Client& http_client);

  void SetAllowedUrlsExtra(std::vector<std::string>&& urls);

 private:
  clients::http::Client* http_client_{nullptr};
};

}  // namespace testsuite

USERVER_NAMESPACE_END
