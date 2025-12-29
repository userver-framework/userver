#include <userver/dynamic_config/updater/component_list.hpp>

#include <userver/clients/http/component.hpp>
#include <userver/dynamic_config/client/component.hpp>
#include <userver/dynamic_config/updater/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config::updater {

components::ComponentList ComponentList() {
    return components::ComponentList()
        .Append<components::HttpClient>("dynamic-config-http-client")
        .Append<components::DynamicConfigClient>()
        .Append<components::DynamicConfigClientUpdater>();
}

}  // namespace dynamic_config::updater

USERVER_NAMESPACE_END
