#pragma once

#include "auth_checker_base.hpp"

namespace server {
namespace handlers {
namespace auth {

class AuthCheckerAllowAny : public AuthCheckerBase {
 public:
  void CheckAuth(const http::HttpRequest&) const override {}
};

}  // namespace auth
}  // namespace handlers
}  // namespace server
