#include <userver/server/http/http_error.hpp>

#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

using handlers::HandlerErrorCode;

constexpr utils::TrivialBiMap kCustomHandlerStatusToHttp = [](auto selector) {
  return selector()
      .Case(HandlerErrorCode::kClientError, HttpStatus::kBadRequest)
      .Case(HandlerErrorCode::kUnauthorized, HttpStatus::kUnauthorized)
      .Case(HandlerErrorCode::kForbidden, HttpStatus::kForbidden)
      .Case(HandlerErrorCode::kResourceNotFound, HttpStatus::kNotFound)
      .Case(HandlerErrorCode::kInvalidUsage, HttpStatus::kMethodNotAllowed)
      .Case(HandlerErrorCode::kNotAcceptable, HttpStatus::kNotAcceptable)
      .Case(HandlerErrorCode::kConflictState, HttpStatus::kConflict)
      .Case(HandlerErrorCode::kPayloadTooLarge, HttpStatus::kPayloadTooLarge)
      .Case(HandlerErrorCode::kTooManyRequests, HttpStatus::kTooManyRequests)
      .Case(HandlerErrorCode::kServerSideError,
            HttpStatus::kInternalServerError)
      .Case(HandlerErrorCode::kBadGateway, HttpStatus::kBadGateway)
      .Case(HandlerErrorCode::kGatewayTimeout, HttpStatus::kGatewayTimeout)
      .Case(HandlerErrorCode::kUnsupportedMediaType,
            HttpStatus::kUnsupportedMediaType);
};

}  // namespace

HttpStatus GetHttpStatus(handlers::HandlerErrorCode code) noexcept {
  if (const auto status = kCustomHandlerStatusToHttp.TryFind(code)) {
    return *status;
  }
  if (code < handlers::HandlerErrorCode::kServerSideError)
    return HttpStatus::kBadRequest;
  return HttpStatus::kInternalServerError;
}

HttpStatus GetHttpStatus(
    const handlers::CustomHandlerException& exception) noexcept {
  if (const auto* const http_exception =
          dynamic_cast<const http::CustomHandlerException*>(&exception)) {
    return http_exception->GetHttpStatus();
  } else {
    return GetHttpStatus(exception.GetCode());
  }
}

}  // namespace server::http

USERVER_NAMESPACE_END
