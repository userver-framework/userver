#include <userver/tracing/set_throttle_reason.hpp>

#include <userver/tracing/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

void SetThrottleReason(std::string value) {
  SetThrottleReason(Span::CurrentSpan(), std::move(value));
}

void SetThrottleReason(Span& span, std::string&& value) {
  span.AddTag("throttle_reason", std::move(value));
}

}  // namespace tracing

USERVER_NAMESPACE_END
