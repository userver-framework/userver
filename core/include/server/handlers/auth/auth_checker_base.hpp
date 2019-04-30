#pragma once

#include <memory>
#include <string>

#include <boost/optional.hpp>

#include <server/auth/user_auth_info.hpp>
#include <server/handlers/exceptions.hpp>
#include <server/http/http_request.hpp>
#include <server/request/request_context.hpp>

namespace server {
namespace handlers {
namespace auth {

struct AuthCheckResult {
  enum class Status {
    kTokenNotFound,
    kInternalCheckFailure,
    kInvalidToken,
    kForbidden,
    kOk
  };

  Status status{Status::kOk};
  boost::optional<std::string> reason{};
  boost::optional<std::string> ext_reason{};
  boost::optional<HandlerErrorCode> code{};
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

}  // namespace auth
}  // namespace handlers
}  // namespace server
