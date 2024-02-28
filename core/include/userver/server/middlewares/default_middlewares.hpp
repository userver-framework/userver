#pragma once

/// @file userver/server/middleware/default_middlewares.hpp
/// @brief @copybrief server::middlewares::DefaultMiddlewaresList()

USERVER_NAMESPACE_BEGIN

namespace components {
class ComponentList;
}

namespace server::middlewares {

/// @ingroup userver_middlewares
///
/// @brief Returns a list of middleware-components required to start a http
/// server.
///
/// The list contains a bunch of middlewares into which most of
/// http-handler functionality is split.
components::ComponentList DefaultMiddlewaresList();

}  // namespace server::middlewares

USERVER_NAMESPACE_END
