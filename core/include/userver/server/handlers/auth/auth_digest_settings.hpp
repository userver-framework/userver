#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

struct AuthDigestSettings {
  std::string algorithm;
  std::vector<std::string> domains;
  std::vector<std::string> qops;
  bool is_proxy;
  bool is_session;
  std::chrono::milliseconds nonce_ttl;
};

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
