#pragma once

#include "handler_config.hpp"

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <server/handlers/exceptions.hpp>
#include <server/request/request_base.hpp>
#include <server/request/request_context.hpp>

namespace server {
namespace handlers {

class HandlerBase : public components::ComponentBase {
 public:
  HandlerBase(const components::ComponentConfig& config,
              const components::ComponentContext& component_context,
              bool is_monitor = false);
  ~HandlerBase() noexcept override = default;

  virtual void HandleRequest(const request::RequestBase& request,
                             request::RequestContext& context) const = 0;
  virtual void OnRequestComplete(const request::RequestBase& request,
                                 request::RequestContext& context) const = 0;

  virtual void HandleReadyRequest(const request::RequestBase&) const {}

  bool IsMonitor() const { return is_monitor_; }

  const HandlerConfig& GetConfig() const;

  bool IsEnabled() const { return is_enabled_; }

 protected:
  // Pull the type names in the handler's scope to shorten throwing code
  using HandlerErrorCode = handlers::HandlerErrorCode;
  using InternalMessage = handlers::InternalMessage;
  using ExternalBody = handlers::ExternalBody;

  using ClientError = handlers::ClientError;
  using InternalServerError = handlers::InternalServerError;

 private:
  HandlerConfig config_;
  bool is_monitor_;
  bool is_enabled_;
};

}  // namespace handlers
}  // namespace server
