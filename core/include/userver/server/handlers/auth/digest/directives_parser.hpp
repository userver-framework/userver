#pragma once

/// @file userver/server/handlers/auth/digest/directives_parser.hpp
/// @brief @copybrief server::handlers::auth::digest::Parser

#include <array>
#include <string>
#include <string_view>

#include "context.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::digest {

/// @brief Class for parsing Authorization header directives from client
/// request.
class Parser final {
 public:
  /// Function to call to parse Authorization header directives.
  ContextFromClient ParseAuthInfo(std::string_view auth_header_value);

 private:
  void PushToClientContext(std::string&& directive, std::string&& value,
                           ContextFromClient& client_context);
  void CheckMandatoryDirectivesPresent() const;
  void CheckDuplicateDirectivesExist() const;

  std::array<std::size_t, kMaxClientDirectivesNumber> directives_counter_{};
};

}  // namespace server::handlers::auth::digest

USERVER_NAMESPACE_END
