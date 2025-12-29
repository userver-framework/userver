#pragma once

/// @file userver/dynamic_config/updater/component_list.hpp
/// @brief @copybrief dynamic_config::updater::ComponentList()

#include <userver/components/component_list.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config::updater {

/// @ingroup userver_components
///
/// @brief Returns list of components required to update configs in runtime.
///
/// The list contains:
/// * @ref components::DynamicConfigClient
/// * @ref components::DynamicConfigClientUpdater
/// The list does not contain components from @ref clients::http::ComponentList and @ref
/// components::MinimalComponentList
components::ComponentList ComponentList();

}  // namespace dynamic_config::updater

USERVER_NAMESPACE_END
