#pragma once

/// @file userver/clients/http/component_list.hpp
/// @brief @copybrief clients::http::ComponentList()

#include <userver/components/component_list.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

/// @ingroup userver_components
///
/// @brief Returns list of components required to use HTTP client.
///
/// The list contains:
/// * @ref components::HttpClientCore
/// * @ref components::HttpClient
/// The list does not contain components from @ref components::MinimalComponentList
components::ComponentList ComponentList();

}  // namespace clients::http

USERVER_NAMESPACE_END
