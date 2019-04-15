#pragma once

#include <memory>
#include <string>

#include <boost/optional.hpp>

#include <server/handlers/exceptions.hpp>
#include <server/http/http_request.hpp>
#include <server/request/request_context.hpp>

namespace server {
namespace handlers {
namespace auth {

class AuthCheckResult {
 public:
  enum class Status {
    kTokenNotFound,
    kInternalCheckFailure,
    kInvalidToken,
    kForbidden,
    kOk
  };

  AuthCheckResult() = default;
  explicit AuthCheckResult(
      Status status, boost::optional<std::string> reason = boost::none,
      boost::optional<std::string> ext_reason = boost::none,
      boost::optional<HandlerErrorCode> code = boost::none);
  virtual ~AuthCheckResult() noexcept = default;

  Status GetStatus() const;

  virtual const std::string& GetDefaultReasonForStatus(Status status) const;

  virtual void RaiseForStatus() const;

 private:
  Status status_{Status::kOk};
  boost::optional<std::string> reason_;
  boost::optional<std::string> ext_reason_;
  boost::optional<HandlerErrorCode> code_;
};

class AuthCheckerBase {
 public:
  virtual ~AuthCheckerBase() noexcept = default;

  [[nodiscard]] virtual AuthCheckResult CheckAuth(
      const http::HttpRequest& request,
      request::RequestContext& context) const = 0;
};

using AuthCheckerBasePtr = std::shared_ptr<AuthCheckerBase>;

}  // namespace auth
}  // namespace handlers
}  // namespace server
