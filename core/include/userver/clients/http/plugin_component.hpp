#pragma once

#include <userver/clients/http/plugin.hpp>
#include <userver/components/loggable_component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::plugin {

class ComponentBase : public components::LoggableComponentBase {
 public:
  using LoggableComponentBase::LoggableComponentBase;
  virtual Plugin& GetPlugin() = 0;
};

}  // namespace clients::http::plugin

USERVER_NAMESPACE_END
