#pragma once

#include <userver/components/component_list.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::middlewares {

void AppendDefaultMiddlewares(components::ComponentList& component_list);

}

USERVER_NAMESPACE_END
