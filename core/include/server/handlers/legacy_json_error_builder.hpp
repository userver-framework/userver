#pragma once

#include <boost/optional.hpp>

#include <formats/json/value.hpp>

#include <server/handlers/exceptions.hpp>
#include <server/http/http_status.hpp>

namespace server {
namespace handlers {

/// Legacy JSON error message builder.
/// Consider using JsonErrorBuilder instead
class LegacyJsonErrorBuilder {
 public:
  static constexpr bool kIsExternalBodyFormatted = true;

  explicit LegacyJsonErrorBuilder(const CustomHandlerException& ex);

  LegacyJsonErrorBuilder(
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
