#pragma once

#include <memory>
#include <optional>
#include <string>

#include <userver/server/auth/user_auth_info.hpp>
#include <userver/server/handlers/exceptions.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

struct AuthCheckResult {
  enum class Status {
    kTokenNotFound,
    kInternalCheckFailure,
    kInvalidToken,
    kForbidden,
    kOk
  };

  Status status{Status::kOk};
  std::optional<std::string> reason{};
  std::optional<std::string> ext_reason{};
  std::optional<HandlerErrorCode> code{};
};

const std::string& GetDefaultReasonForStatus(AuthCheckResult::Status status);
void RaiseForStatus(const AuthCheckResult& auth_check);

class AuthCheckerBase {
 public:
  virtual ~AuthCheckerBase();

  [[nodiscard]] virtual AuthCheckResult CheckAuth(
      const http::HttpRequest& request,
      request::RequestContext& context) const = 0;

  [[nodiscard]] virtual bool SupportsUserAuth() const noexcept = 0;

 protected:
  void SetUserAuthInfo(server::request::RequestContext& request_context,
                       server::auth::UserAuthInfo&& info) const;
};

using AuthCheckerBasePtr = std::shared_ptr<AuthCheckerBase>;

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
