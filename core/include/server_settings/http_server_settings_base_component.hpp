#pragma once

#include <components/component_base.hpp>

namespace server {
namespace handlers {
namespace auth {

class AuthCheckerSettings;

}  // namespace auth
}  // namespace handlers
}  // namespace server

namespace components {

class HttpServerSettingsBase : public LoggableComponentBase {
 public:
  using LoggableComponentBase::LoggableComponentBase;

  ~HttpServerSettingsBase() override = default;

  static constexpr const char* kName = "http-server-settings";

  virtual bool NeedLogRequest() const = 0;
  virtual bool NeedLogRequestHeaders() const = 0;
  virtual bool NeedCheckAuthInHandlers() const = 0;

  virtual const server::handlers::auth::AuthCheckerSettings&
  GetAuthCheckerSettings() const = 0;
};

}  // namespace components
