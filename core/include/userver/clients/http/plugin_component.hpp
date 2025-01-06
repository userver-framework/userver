#pragma once

#include <userver/clients/http/plugin.hpp>
#include <userver/components/component_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::plugin {

class ComponentBase : public components::ComponentBase {
public:
    using components::ComponentBase::ComponentBase;

    virtual Plugin& GetPlugin() = 0;
};

}  // namespace clients::http::plugin

USERVER_NAMESPACE_END
