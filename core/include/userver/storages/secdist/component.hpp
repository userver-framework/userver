#pragma once

/// @file userver/storages/secdist/component.hpp
/// @brief @copybrief components::Secdist

#include <string>

#include <userver/components/component_base.hpp>
#include <userver/storages/secdist/secdist.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// @brief Component that stores security related data (keys, passwords, ...).
///
/// The component must be configured in service config.
///
/// Secdist requires a provider @ref storages::secdist::SecdistProvider
/// You can implement your own or use @ref components::DefaultSecdistProvider.
///
/// ## Static configuration example:
///
/// @snippet samples/redis_service/static_config.yaml Sample secdist static config
///
/// ## Static options of components::Secdist :
/// @include{doc} scripts/docs/en/components_schema/core/src/storages/secdist/component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
class Secdist final : public ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of @ref components::Secdist
    static constexpr std::string_view kName = "secdist";

    Secdist(const ComponentConfig&, const ComponentContext&);

    const storages::secdist::SecdistConfig& Get() const;

    rcu::ReadablePtr<storages::secdist::SecdistConfig> GetSnapshot() const;

    storages::secdist::Secdist& GetStorage();

    static yaml_config::Schema GetStaticConfigSchema();

private:
    storages::secdist::Secdist secdist_;
};

template <>
inline constexpr bool kHasValidate<Secdist> = true;

template <>
inline constexpr auto kConfigFileMode<Secdist> = ConfigFileMode::kNotRequired;

}  // namespace components

USERVER_NAMESPACE_END
