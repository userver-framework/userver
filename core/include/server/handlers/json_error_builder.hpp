#pragma once

#include <server/handlers/exceptions.hpp>

namespace server {
namespace handlers {

/// JSON error message builder.
/// Useful for handlers derived from HttpHandlerBase but responding via JSON.
class JsonErrorBuilder {
 public:
  explicit JsonErrorBuilder(const CustomHandlerException& ex);

  const std::string& GetInternalMessage() const { return internal_message; };
  const std::string& GetExternalBody() const { return json_error_body; }

 private:
  std::string internal_message;
  std::string json_error_body;
};

}  // namespace handlers
}  // namespace server
