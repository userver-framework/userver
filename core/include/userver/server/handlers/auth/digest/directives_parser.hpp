#pragma once

/// @file userver/server/handlers/auth/digest/directives_parser.hpp
/// @brief @copybrief server::handlers::auth::digest::Parser

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <userver/server/handlers/auth/digest/context.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::digest {

/// @brief Class for parsing Authorization header directives from client
/// request.
class Parser {
 public:
  /// Function to call to parse Authorization header value into directive-value
  /// map
  void ParseAuthInfo(std::string_view header_value);
  /// Function to call to get client digest context from directive-value map
  ContextFromClient GetClientContext();

 private:
  std::unordered_map<std::string, std::string> directive_mapping;
};

}  // namespace server::handlers::auth::digest

USERVER_NAMESPACE_END
