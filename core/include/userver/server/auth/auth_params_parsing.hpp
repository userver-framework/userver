#pragma once

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

#include "digestcontext.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

struct Auth {
  static std::unordered_set<std::string> mandatory_directives;
};

struct ClientAuth : public Auth {
  std::unordered_set<std::string> mandatory_directives = {
      "realm", "nonce", "response", "uri", "username"};
};

struct ServerAuth : public Auth {
  std::unordered_set<std::string> mandatory_directives = {"realm", "nonce"};
};

class DigestParsing {
  ClientAuth client_params;
  ServerAuth server_params;
  formats::json::ValueBuilder directive_mapping;

 public:
  void ParseAuthInfo(std::string_view header_value);
  DigestContextFromClient GetClientContext();
  DigestContextFromServer GetServerContext();
};

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END