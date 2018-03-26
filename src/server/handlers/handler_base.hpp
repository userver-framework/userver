#pragma once

#include "handler_config.hpp"
#include "handler_context.hpp"

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <server/request/request_base.hpp>

namespace server {
namespace handlers {

class HandlerBase : public components::ComponentBase {
 public:
  HandlerBase(const components::ComponentConfig& config,
              const components::ComponentContext& component_context);
  virtual ~HandlerBase() {}

  virtual void HandleRequest(const server::request::RequestBase& request,
                             HandlerContext& context) const noexcept = 0;
  virtual void OnRequestComplete(const server::request::RequestBase& request,
                                 HandlerContext& context) const noexcept = 0;

  const HandlerConfig& GetConfig() const;

 private:
  HandlerConfig config_;
};

}  // namespace handlers
}  // namespace server
