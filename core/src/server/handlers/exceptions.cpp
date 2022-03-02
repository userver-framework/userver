#include <userver/server/handlers/exceptions.hpp>

#include <unordered_map>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {

const std::unordered_map<HandlerErrorCode, std::string, HandlerErrorCodeHash>
    kCodeDescriptions{
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
    };

const std::unordered_map<HandlerErrorCode, std::string, HandlerErrorCodeHash>
    kFallbackServiceCodes{
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
    };

}  // namespace

std::string GetCodeDescription(HandlerErrorCode code) {
  if (auto f = kCodeDescriptions.find(code); f != kCodeDescriptions.end()) {
    return f->second;
  }
  return kCodeDescriptions.at(HandlerErrorCode::kUnknownError);
}

std::string GetFallbackServiceCode(HandlerErrorCode code) {
  auto it = kFallbackServiceCodes.find(code);
  if (it == kFallbackServiceCodes.end()) {
    it = kCodeDescriptions.find(HandlerErrorCode::kUnknownError);
  }
  return it->second;
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
