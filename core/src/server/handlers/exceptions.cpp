#include <server/handlers/exceptions.hpp>

#include <unordered_map>

namespace server {
namespace handlers {

namespace {

const std::unordered_map<HandlerErrorCode, const char*, HandlerErrorCodeHash>
    kCodeDescriptions{
        {HandlerErrorCode::kUnknownError, "Unknown error"},
        {HandlerErrorCode::kClientError, "Client error"},
        {HandlerErrorCode::kUnauthorized, "Unauthorized"},
        {HandlerErrorCode::kRequestParseError, "Bad request"},
        {HandlerErrorCode::kServerSideError, "Internal server error"}};

}  // namespace

const char* GetCodeDescription(HandlerErrorCode code) noexcept {
  if (auto f = kCodeDescriptions.find(code); f != kCodeDescriptions.end()) {
    return f->second;
  }
  return kCodeDescriptions.at(HandlerErrorCode::kUnknownError);
}

}  // namespace handlers
}  // namespace server
