#pragma once

#include <optional>
#include <string>

#include <userver/http/content_type.hpp>

namespace server::handlers {

struct FormattedErrorData {
  std::string external_body;
  std::optional<::http::ContentType> content_type{};
};

}  // namespace server::handlers
