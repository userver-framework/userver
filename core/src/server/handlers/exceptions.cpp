#include <userver/server/handlers/exceptions.hpp>

#include <userver/utils/consteval_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {

constexpr auto kCodeDescriptions =
    utils::MakeConsinitMap<HandlerErrorCode, std::string_view>({
        {HandlerErrorCode::kUnknownError, "Unknown error"},
        {HandlerErrorCode::kClientError, "Client error"},
        {HandlerErrorCode::kRequestParseError, "Bad request"},
        {HandlerErrorCode::kUnauthorized, "Unauthorized"},
        {HandlerErrorCode::kForbidden, "Forbidden"},
        {HandlerErrorCode::kResourceNotFound, "Not found"},
        {HandlerErrorCode::kInvalidUsage, "Invalid usage"},
        {HandlerErrorCode::kNotAcceptable, "Not acceptable"},
        {HandlerErrorCode::kConflictState, "Conflict"},
        {HandlerErrorCode::kPayloadTooLarge, "Payload too large"},
        {HandlerErrorCode::kTooManyRequests, "Too many requests"},
        {HandlerErrorCode::kServerSideError, "Internal server error"},
        {HandlerErrorCode::kBadGateway, "Bad gateway"},
        {HandlerErrorCode::kGatewayTimeout, "Gateway Timeout"},
    });

constexpr auto kFallbackServiceCodes =
    utils::MakeConsinitMap<HandlerErrorCode, std::string_view>({
        {HandlerErrorCode::kUnknownError, "unknown"},
        {HandlerErrorCode::kClientError, "client_error"},
        {HandlerErrorCode::kRequestParseError, "bad_request"},
        {HandlerErrorCode::kUnauthorized, "unauthorized"},
        {HandlerErrorCode::kForbidden, "forbidden"},
        {HandlerErrorCode::kResourceNotFound, "not_found"},
        {HandlerErrorCode::kInvalidUsage, "invalid_usage"},
        {HandlerErrorCode::kNotAcceptable, "not_acceptable"},
        {HandlerErrorCode::kConflictState, "conflict"},
        {HandlerErrorCode::kPayloadTooLarge, "payload_too_large"},
        {HandlerErrorCode::kTooManyRequests, "too_many_requests"},
        {HandlerErrorCode::kServerSideError, "internal_server_error"},
        {HandlerErrorCode::kBadGateway, "bad_gateway"},
        {HandlerErrorCode::kGatewayTimeout, "gateway_timeout"},
    });

}  // namespace

std::string GetCodeDescription(HandlerErrorCode code) {
  auto ptr = kCodeDescriptions.FindOrNullptr(code);
  if (!ptr) {
    ptr = kCodeDescriptions.FindOrNullptr(HandlerErrorCode::kUnknownError);
  }

  return std::string{*ptr};
}

std::string GetFallbackServiceCode(HandlerErrorCode code) {
  auto ptr = kFallbackServiceCodes.FindOrNullptr(code);
  if (!ptr) {
    ptr = kCodeDescriptions.FindOrNullptr(HandlerErrorCode::kUnknownError);
  }
  return std::string{*ptr};
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
