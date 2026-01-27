#pragma once

/// @file userver/components/fs_cache.hpp
/// @brief @copybrief components::FsCache

#include <userver/components/component_base.hpp>
#include <userver/fs/fs_cache_client.hpp>
#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// @brief Component for storing files in memory
///
/// ## Static options of components::FsCache :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/fs_cache.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
class FsCache final : public components::ComponentBase {
public:
    using Client = fs::FsCacheClient;

    FsCache(const components::ComponentConfig& config, const components::ComponentContext& context);

    static yaml_config::Schema GetStaticConfigSchema();

    const Client& GetClient() const;

private:
    Client client_;
};

template <>
inline constexpr bool kHasValidate<FsCache> = true;

}  // namespace components

USERVER_NAMESPACE_END
