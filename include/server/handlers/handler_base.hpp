#pragma once

#include "handler_config.hpp"

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <server/request/request_base.hpp>
#include <server/request/request_context.hpp>

namespace server {
namespace handlers {

class HandlerBase : public components::ComponentBase {
 public:
  HandlerBase(const components::ComponentConfig& config,
              const components::ComponentContext& component_context,
              bool is_monitor = false);
  virtual ~HandlerBase() {}

  virtual void HandleRequest(const request::RequestBase& request,
                             request::RequestContext& context) const
      noexcept = 0;
  virtual void OnRequestComplete(const request::RequestBase& request,
                                 request::RequestContext& context) const
      noexcept = 0;

  bool IsMonitor() const { return is_monitor_; }

  const HandlerConfig& GetConfig() const;

  bool IsEnabled() const { return is_enabled_; }

 private:
  HandlerConfig config_;
  bool is_monitor_;
  bool is_enabled_;
};

}  // namespace handlers
}  // namespace server
