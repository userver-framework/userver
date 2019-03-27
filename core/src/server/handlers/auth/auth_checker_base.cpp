#include <server/handlers/auth/auth_checker_base.hpp>

#include <stdexcept>

#include <logging/log.hpp>

namespace server {
namespace handlers {
namespace auth {

AuthCheckResult::AuthCheckResult(Status status,
                                 boost::optional<std::string> reason,
                                 boost::optional<std::string> ext_reason,
                                 boost::optional<HandlerErrorCode> code)
    : status_(status),
      reason_(std::move(reason)),
      ext_reason_(std::move(ext_reason)),
      code_(std::move(code)) {}

AuthCheckResult::Status AuthCheckResult::GetStatus() const { return status_; }

const std::string& AuthCheckResult::GetDefaultReasonForStatus(
    Status status) const {
  static const std::string kTokenNotFoundReason = "token not found";
  static const std::string kInternalCheckFailureReason = "auth check failed";
  static const std::string kInvalidTokenReason = "invalid token";
  static const std::string kForbiddenReason = "forbidden";
  static const std::string kOkReason = "OK";

  switch (status_) {
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

void AuthCheckResult::RaiseForStatus() const {
  const std::string& reason =
      reason_ ? *reason_ : GetDefaultReasonForStatus(status_);

  if (code_) {
    throw CustomHandlerException(
        ext_reason_ ? ExternalBody{*ext_reason_} : ExternalBody{},
        InternalMessage{reason}, *code_);
  }

  switch (status_) {
    case Status::kTokenNotFound:
      throw ClientError(
          InternalMessage{reason},
          ext_reason_ ? ExternalBody{*ext_reason_} : ExternalBody{});
    case Status::kInvalidToken:
      throw Unauthorized(
          InternalMessage{reason},
          ext_reason_ ? ExternalBody{*ext_reason_} : ExternalBody{});
    case Status::kForbidden:
      throw CustomHandlerException(
          ext_reason_ ? ExternalBody{*ext_reason_} : ExternalBody{},
          InternalMessage{reason}, HandlerErrorCode::kForbidden);
    case Status::kInternalCheckFailure:
      throw InternalServerError(
          InternalMessage{reason},
          ext_reason_ ? ExternalBody{*ext_reason_} : ExternalBody{});
    case Status::kOk:
      LOG_DEBUG() << "authentication succeeded: " << reason;
      break;
  }
}

}  // namespace auth
}  // namespace handlers
}  // namespace server
