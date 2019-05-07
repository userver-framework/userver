#pragma once

#include <boost/optional.hpp>

#include <formats/json/value.hpp>

#include <server/handlers/exceptions.hpp>
#include <server/http/http_status.hpp>

namespace server {
namespace handlers {

/// JSON error message builder.
/// Useful for handlers derived from HttpHandlerBase but responding via JSON.
class JsonErrorBuilder {
 public:
  explicit JsonErrorBuilder(const CustomHandlerException& ex);

  JsonErrorBuilder(
      http::HttpStatus status, std::string internal_message,
      std::string external_error_body,
      boost::optional<const formats::json::Value&> details = boost::none);

  const std::string& GetInternalMessage() const { return internal_message_; };
  const std::string& GetExternalBody() const { return json_error_body_; }

 private:
  std::string internal_message_;
  std::string json_error_body_;
};

}  // namespace handlers
}  // namespace server
