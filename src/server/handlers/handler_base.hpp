#pragma once

#include "handler_config.hpp"

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <server/request/request_base.hpp>
#include <server/request/request_context.hpp>

namespace server {
namespace handlers {

class HandlerBase : public components::ComponentBase {
 public:
  HandlerBase(const components::ComponentConfig& config,
              const components::ComponentContext& component_context);
  virtual ~HandlerBase() {}

  virtual void HandleRequest(const request::RequestBase& request,
                             request::RequestContext& context) const
      noexcept = 0;
  virtual void OnRequestComplete(const request::RequestBase& request,
                                 request::RequestContext& context) const
      noexcept = 0;

  virtual bool IsMonitor() const { return false; }

  const HandlerConfig& GetConfig() const;

 private:
  HandlerConfig config_;
};

}  // namespace handlers
}  // namespace server
