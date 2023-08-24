#include <userver/server/handlers/exceptions.hpp>

#include <userver/server/http/http_error.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {

constexpr utils::TrivialBiMap kCodeDescriptions = [](auto selector) {
  return selector()
      .Case(HandlerErrorCode::kUnknownError, "Unknown error")
      .Case(HandlerErrorCode::kClientError, "Client error")
      .Case(HandlerErrorCode::kRequestParseError, "Bad request")
      .Case(HandlerErrorCode::kUnauthorized, "Unauthorized")
      .Case(HandlerErrorCode::kForbidden, "Forbidden")
      .Case(HandlerErrorCode::kResourceNotFound, "Not found")
      .Case(HandlerErrorCode::kInvalidUsage, "Invalid usage")
      .Case(HandlerErrorCode::kNotAcceptable, "Not acceptable")
      .Case(HandlerErrorCode::kConflictState, "Conflict")
      .Case(HandlerErrorCode::kPayloadTooLarge, "Payload too large")
      .Case(HandlerErrorCode::kTooManyRequests, "Too many requests")
      .Case(HandlerErrorCode::kServerSideError, "Internal server error")
      .Case(HandlerErrorCode::kBadGateway, "Bad gateway")
      .Case(HandlerErrorCode::kGatewayTimeout, "Gateway Timeout");
};

constexpr utils::TrivialBiMap kFallbackServiceCodes = [](auto selector) {
  return selector()
      .Case(HandlerErrorCode::kUnknownError, "unknown")
      .Case(HandlerErrorCode::kClientError, "client_error")
      .Case(HandlerErrorCode::kRequestParseError, "bad_request")
      .Case(HandlerErrorCode::kUnauthorized, "unauthorized")
      .Case(HandlerErrorCode::kForbidden, "forbidden")
      .Case(HandlerErrorCode::kResourceNotFound, "not_found")
      .Case(HandlerErrorCode::kInvalidUsage, "invalid_usage")
      .Case(HandlerErrorCode::kNotAcceptable, "not_acceptable")
      .Case(HandlerErrorCode::kConflictState, "conflict")
      .Case(HandlerErrorCode::kPayloadTooLarge, "payload_too_large")
      .Case(HandlerErrorCode::kTooManyRequests, "too_many_requests")
      .Case(HandlerErrorCode::kServerSideError, "internal_server_error")
      .Case(HandlerErrorCode::kBadGateway, "bad_gateway")
      .Case(HandlerErrorCode::kGatewayTimeout, "gateway_timeout");
};

}  // namespace

std::string_view GetCodeDescription(HandlerErrorCode code) {
  return kCodeDescriptions.TryFind(code).value_or(
      kCodeDescriptions.TryFind(HandlerErrorCode::kUnknownError).value());
}

std::string_view GetFallbackServiceCode(HandlerErrorCode code) {
  return kFallbackServiceCodes.TryFind(code).value_or(
      kFallbackServiceCodes.TryFind(HandlerErrorCode::kUnknownError).value());
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
