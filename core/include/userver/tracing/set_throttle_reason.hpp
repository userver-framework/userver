#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace tracing {

class Span;

/// Sets a Span tag with overload reason
void SetThrottleReason(std::string value);

/// @overload void SetThrottleReason(std::string_view value);
void SetThrottleReason(Span& span, std::string&& value);

}  // namespace tracing

USERVER_NAMESPACE_END
