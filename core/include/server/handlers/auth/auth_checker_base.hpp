#pragma once

#include <memory>

#include <server/http/http_request.hpp>

namespace server {
namespace handlers {
namespace auth {

class AuthCheckerBase {
 public:
  virtual ~AuthCheckerBase() {}

  virtual void CheckAuth(const http::HttpRequest& request) const = 0;
};

using AuthCheckerBasePtr = std::shared_ptr<AuthCheckerBase>;

}  // namespace auth
}  // namespace handlers
}  // namespace server
