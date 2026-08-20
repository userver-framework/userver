#pragma once

#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace server::http {

/// @throws std::runtime_error if `name` has a character that RFC 9110 does not
/// allow in a header name.
void CheckHeaderName(std::string_view name);

/// @throws std::runtime_error if `value` has a character that RFC 9110 does not
/// allow in a header value.
void CheckHeaderValue(std::string_view value);

}  // namespace server::http

USERVER_NAMESPACE_END
