#include <userver/server/http/http_error.hpp>

#include <unordered_map>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

using handlers::HandlerErrorCode;

const std::unordered_map<handlers::HandlerErrorCode, HttpStatus,
                         handlers::HandlerErrorCodeHash>
    kCustomHandlerStatusToHttp{
        {HandlerErrorCode::kClientError, HttpStatus::kBadRequest},
        {HandlerErrorCode::kUnauthorized, HttpStatus::kUnauthorized},
        {HandlerErrorCode::kForbidden, HttpStatus::kForbidden},
        {HandlerErrorCode::kResourceNotFound, HttpStatus::kNotFound},
        {HandlerErrorCode::kInvalidUsage, HttpStatus::kMethodNotAllowed},
        {HandlerErrorCode::kNotAcceptable, HttpStatus::kNotAcceptable},
        {HandlerErrorCode::kConflictState, HttpStatus::kConflict},
        {HandlerErrorCode::kPayloadTooLarge, HttpStatus::kPayloadTooLarge},
        {HandlerErrorCode::kTooManyRequests, HttpStatus::kTooManyRequests},
        {HandlerErrorCode::kServerSideError, HttpStatus::kInternalServerError},
        {HandlerErrorCode::kBadGateway, HttpStatus::kBadGateway},
        {HandlerErrorCode::kGatewayTimeout, HttpStatus::kGatewayTimeout},
        {HandlerErrorCode::kUnsupportedMediaType,
         HttpStatus::kUnsupportedMediaType},
    };

}  // namespace

HttpStatus GetHttpStatus(handlers::HandlerErrorCode code) noexcept {
  if (auto f = kCustomHandlerStatusToHttp.find(code);
      f != kCustomHandlerStatusToHttp.end()) {
    return f->second;
  }
  if (code < handlers::HandlerErrorCode::kServerSideError)
    return HttpStatus::kBadRequest;
  return HttpStatus::kInternalServerError;
}

}  // namespace server::http

USERVER_NAMESPACE_END
