#pragma once

/// @file userver/storages/rocks/component.hpp
/// @brief @copybrief rocks::Rocks

#include <userver/components/component_base.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/storages/rocks/client_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::rocks {

/// @ingroup userver_components
///
/// @brief RocksDB client component.
///
/// ## Static options of storages::rocks::Component :
/// @include{doc} scripts/docs/en/components_schema/rocks/src/storages/rocks/component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
class Component : public components::ComponentBase {
public:
    Component(const components::ComponentConfig&, const components::ComponentContext&);

    ~Component() = default;

    storages::rocks::ClientPtr MakeClient();

    static yaml_config::Schema GetStaticConfigSchema();

private:
    storages::rocks::ClientPtr client_ptr_;
};

}  // namespace storages::rocks

USERVER_NAMESPACE_END
