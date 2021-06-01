#pragma once

#include <components/component_list.hpp>
#include <server_settings/http_server_settings_component.hpp>

namespace server_settings {
namespace impl {

components::ComponentList ServerCommonComponentList();

}  // namespace impl

/// Deprecated, use components::CommonServerComponentList instead
template <typename HttpServerSettingsConfigNames = ConfigNamesDefault>
components::ComponentList ServerCommonComponentList() {
  return impl::ServerCommonComponentList()
      .Append<components::HttpServerSettings<HttpServerSettingsConfigNames>>();
}

}  // namespace server_settings
