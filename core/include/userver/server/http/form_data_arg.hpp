#pragma once

/// @file userver/server/http/form_data_arg.hpp
/// @brief @copybrief server::http::FormDataArg

#include <optional>
#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace server::http {

/// @brief Argument of a multipart/form-data request
struct FormDataArg {
  std::string_view value;
  std::string_view content_disposition;
  std::optional<std::string> filename;
  std::optional<std::string> default_charset;
  std::optional<std::string_view> content_type;

  bool operator==(const FormDataArg& r) const {
    return value == r.value && content_disposition == r.content_disposition &&
           filename == r.filename && default_charset == r.default_charset &&
           content_type == r.content_type;
  }

  std::string Charset() const;

  std::string ToDebugString() const;
};

}  // namespace server::http

USERVER_NAMESPACE_END
