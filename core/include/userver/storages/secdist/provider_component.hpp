#pragma once

/// @file userver/storages/secdist/component.hpp
/// @brief @copybrief components::DefaultSecdistProvider

#include <string>

#include <userver/components/component_base.hpp>
#include <userver/storages/secdist/provider.hpp>
#include <userver/storages/secdist/secdist.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::secdist {

class DefaultLoader final : public storages::secdist::SecdistProvider {
public:
    struct Settings {
        std::string config_path;
        SecdistFormat format{SecdistFormat::kJson};
        bool missing_ok{false};
        std::optional<std::string> environment_secrets_key;
        engine::TaskProcessor* blocking_task_processor{nullptr};
        formats::json::Value inline_config;
    };

    explicit DefaultLoader(Settings settings);

    formats::json::Value Get() const override;

private:
    Settings settings_;
};

}  // namespace storages::secdist

namespace components {

/// @ingroup userver_components
///
/// @brief Component that stores security related data (keys, passwords, ...).
///
/// The component must be configured in service config.
///
/// ## Static options of components::DefaultSecdistProvider :
/// @include{doc} scripts/docs/en/components_schema/core/src/storages/secdist/provider_component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
class DefaultSecdistProvider final : public ComponentBase, public storages::secdist::SecdistProvider {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of @ref components::DefaultSecdistProvider
    static constexpr std::string_view kName = "default-secdist-provider";

    DefaultSecdistProvider(const ComponentConfig&, const ComponentContext&);

    formats::json::Value Get() const override;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    storages::secdist::DefaultLoader loader_;
};

template <>
inline constexpr bool kHasValidate<DefaultSecdistProvider> = true;

}  // namespace components

USERVER_NAMESPACE_END
