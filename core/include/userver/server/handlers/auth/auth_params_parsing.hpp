#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <userver/server/handlers/auth/digest_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

class DigestParsing {
 public:
  void ParseAuthInfo(std::string_view header_value);
  DigestContextFromClient GetClientContext();
 private:
  std::unordered_map<std::string, std::string> directive_mapping;
};

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
