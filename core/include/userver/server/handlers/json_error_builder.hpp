#pragma once

#include <userver/formats/json/value.hpp>
#include <userver/http/content_type.hpp>

#include <userver/server/handlers/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

/// JSON error message builder.
/// Useful for handlers derived from HttpHandlerBase but responding via JSON.
class JsonErrorBuilder {
 public:
  static constexpr bool kIsExternalBodyFormatted = true;

  explicit JsonErrorBuilder(const CustomHandlerException& ex);

  JsonErrorBuilder(std::string_view error_code, std::string internal_message,
                   std::string_view external_error_body,
                   formats::json::Value = {});

  const std::string& GetInternalMessage() const { return internal_message_; };

  const std::string& GetExternalBody() const { return json_error_body_; }

  static const USERVER_NAMESPACE::http::ContentType& GetContentType() {
    return USERVER_NAMESPACE::http::content_type::kApplicationJson;
  }

 private:
  std::string internal_message_;
  std::string json_error_body_;
};

}  // namespace server::handlers

USERVER_NAMESPACE_END
