#include <server/handlers/exceptions.hpp>

#include <unordered_map>

namespace server {
namespace handlers {

namespace {

const std::unordered_map<HandlerErrorCode, const char*, HandlerErrorCodeHash>
    kCodeDescriptions{
        {HandlerErrorCode::kUnknownError, "Unknown error"},
        {HandlerErrorCode::kClientError, "Client error"},
        {HandlerErrorCode::kRequestParseError, "Bad request"},
        {HandlerErrorCode::kUnauthorized, "Unauthorized"},
        {HandlerErrorCode::kForbidden, "Forbidden"},
        {HandlerErrorCode::kResourceNotFound, "Not found"},
        {HandlerErrorCode::kInvalidUsage, "Invalid usage"},
        {HandlerErrorCode::kNotAcceptable, "Not acceptable"},
        {HandlerErrorCode::kConfictState, "Conflict"},
        {HandlerErrorCode::kPayloadTooLarge, "Payload too large"},
        {HandlerErrorCode::kTooManyRequests, "Too many requests"},
        {HandlerErrorCode::kServerSideError, "Internal server error"},
        {HandlerErrorCode::kBadGateway, "Bad gateway"},
        {HandlerErrorCode::kGatewayTimeout, "Gateway Timeout"},
    };

}  // namespace

const char* GetCodeDescription(HandlerErrorCode code) noexcept {
  if (auto f = kCodeDescriptions.find(code); f != kCodeDescriptions.end()) {
    return f->second;
  }
  return kCodeDescriptions.at(HandlerErrorCode::kUnknownError);
}

}  // namespace handlers
}  // namespace server
