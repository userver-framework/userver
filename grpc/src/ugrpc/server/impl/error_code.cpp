#include <userver/ugrpc/server/impl/error_code.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

grpc::StatusCode CustomStatusToGrpc(
    USERVER_NAMESPACE::server::handlers::HandlerErrorCode code) {
  using Code = grpc::StatusCode;
  using HCode = USERVER_NAMESPACE::server::handlers::HandlerErrorCode;

  switch (code) {
    case HCode::kUnknownError:
      return Code::UNKNOWN;
    case HCode::kRequestParseError:
    case HCode::kNotAcceptable:
    case HCode::kUnsupportedMediaType:
    case HCode::kInvalidUsage:
      return Code::INVALID_ARGUMENT;
    case HCode::kUnauthorized:
      return Code::UNAUTHENTICATED;
    case HCode::kForbidden:
      return Code::PERMISSION_DENIED;
    case HCode::kResourceNotFound:
      return Code::NOT_FOUND;
    case HCode::kConflictState:
      return Code::ABORTED;
    case HCode::kPayloadTooLarge:
    case HCode::kTooManyRequests:
      return Code::RESOURCE_EXHAUSTED;
    case HCode::kBadGateway:
    case HCode::kGatewayTimeout:
    case HCode::kServerSideError:
      return Code::INTERNAL;
    default:
      return Code::UNKNOWN;
  }
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
