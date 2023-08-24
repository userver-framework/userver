#include <userver/server/handlers/auth/auth_checker_base.hpp>

#include <stdexcept>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

const std::string& GetDefaultReasonForStatus(AuthCheckResult::Status status) {
  using Status = AuthCheckResult::Status;

  static const std::string kTokenNotFoundReason = "token not found";
  static const std::string kInternalCheckFailureReason = "auth check failed";
  static const std::string kInvalidTokenReason = "invalid token";
  static const std::string kForbiddenReason = "forbidden";
  static const std::string kOkReason = "OK";

  switch (status) {
    case Status::kTokenNotFound:
      return kTokenNotFoundReason;
    case Status::kInternalCheckFailure:
      return kInternalCheckFailureReason;
    case Status::kInvalidToken:
      return kInvalidTokenReason;
    case Status::kForbidden:
      return kForbiddenReason;
    case Status::kOk:
      return kOkReason;
  }
  throw std::runtime_error("unknown status: " +
                           std::to_string(static_cast<int>(status)));
}

void RaiseForStatus(const AuthCheckResult& auth_check) {
  using Status = AuthCheckResult::Status;

  ExternalBody ext_body{auth_check.ext_reason.value_or("")};
  InternalMessage reason{auth_check.reason
                             ? *auth_check.reason
                             : GetDefaultReasonForStatus(auth_check.status)};

  if (auth_check.code) {
    throw CustomHandlerException({}, std::move(ext_body), std::move(reason),
                                 *auth_check.code);
  }

  switch (auth_check.status) {
    case Status::kTokenNotFound:
      throw ClientError(std::move(reason), std::move(ext_body));
    case Status::kInvalidToken:
      throw Unauthorized(std::move(reason), std::move(ext_body));
    case Status::kForbidden:
      throw CustomHandlerException({}, std::move(ext_body), std::move(reason),
                                   HandlerErrorCode::kForbidden);
    case Status::kInternalCheckFailure:
      throw InternalServerError(std::move(reason), std::move(ext_body));
    case Status::kOk:
      LOG_DEBUG() << "authentication succeeded: " << reason.body;
      break;
  }
}

AuthCheckerBase::~AuthCheckerBase() = default;

void AuthCheckerBase::SetUserAuthInfo(
    server::request::RequestContext& request_context,
    server::auth::UserAuthInfo&& info) const {
  UASSERT_MSG(SupportsUserAuth(),
              "Attempt to set user info from a handler that does not support "
              "user auth");

  server::auth::UserAuthInfo::Set(request_context, std::move(info));
}

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
