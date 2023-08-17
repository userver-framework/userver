#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <optional>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

struct AuthDigestSettings {
  std::string algorithm;
  std::vector<std::string> qops;
  std::string qop_str;
  std::chrono::milliseconds nonce_ttl;
  std::optional<bool> is_proxy;
  std::optional<bool> is_session;
  std::vector<std::string> domains;
};

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END