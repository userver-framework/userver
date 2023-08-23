#pragma once

/// @file userver/server/handlers/auth/auth_params_parsing.hpp
/// @brief Class for parsing Authorization header directives from client
/// request.

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

/// Class for parsing Authrorization header
class DigestParser {
 public:
  /// Function to call to parse Authorization header value into directive-value
  /// map
  void ParseAuthInfo(std::string_view header_value);
  /// Function to call to get client digest context from directive-value map
  DigestContextFromClient GetClientContext();

 private:
  std::unordered_map<std::string, std::string> directive_mapping;
};

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
