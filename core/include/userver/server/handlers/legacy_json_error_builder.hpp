#pragma once

#include <formats/json/value.hpp>
#include <http/content_type.hpp>

#include <server/handlers/exceptions.hpp>
#include <server/http/http_status.hpp>

namespace server::handlers {

/// Legacy JSON error message builder that returns "code" as an integer.
/// Consider using JsonErrorBuilder instead
class LegacyJsonErrorBuilder {
 public:
  static constexpr bool kIsExternalBodyFormatted = true;

  explicit LegacyJsonErrorBuilder(const CustomHandlerException& ex);

  LegacyJsonErrorBuilder(http::HttpStatus status, std::string internal_message,
                         std::string external_error_body);

  LegacyJsonErrorBuilder(http::HttpStatus status, std::string internal_message,
                         std::string external_error_body,
                         const formats::json::Value& details);

  const std::string& GetInternalMessage() const { return internal_message_; };

  const std::string& GetExternalBody() const { return json_error_body_; }

  static const ::http::ContentType& GetContentType() {
    return ::http::content_type::kApplicationJson;
  }

 private:
  std::string internal_message_;
  std::string json_error_body_;
};

}  // namespace server::handlers
