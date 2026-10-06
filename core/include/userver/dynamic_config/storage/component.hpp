#pragma once

/// @file userver/dynamic_config/storage/component.hpp
/// @brief @copybrief components::DynamicConfig

#include <memory>
#include <string_view>
#include <type_traits>

#include <userver/components/component_base.hpp>
#include <userver/concurrent/async_event_source.hpp>
#include <userver/dynamic_config/impl/snapshot.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/dynamic_config/updates_sink/component.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/resource_scopes_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// @brief Component that stores the
/// @ref scripts/docs/en/userver/dynamic_config.md "dynamic config".
///
/// Note that the service with `updates-enabled: true` and without
/// configs cache requires successful update to start. See
/// @ref dynamic_config_fallback for details and explanation.
///
/// ## Behavior on missing configs
///
/// If a config variable is entirely missing the fetched config, the value from
/// `dynamic_config_fallback.json` is used (see `fallback-path` static config option).
///
/// ## Behavior on config parsing failure
///
/// If a config variable from the fetched config fails to be parsed (the parser
/// fails with an exception), then the whole config update fails. It means that:
///
/// - If the service is just starting, it will shut down
/// - If the service is already running, the config updates will stop until
///   the config in the config service changes to a valid one. You can
///   monitor this situation using the metric at path `cache.any.time.time-from-last-successful-start-ms`
///
/// ## Static options of components::DynamicConfigClient :
/// @include{doc} scripts/docs/en/components_schema/core/src/dynamic_config/storage/component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// ## Static configuration example:
///
/// @snippet core/src/components/common_component_list_test.cpp  Sample dynamic config component config
///
/// ## Usage example:
/// @snippet core/src/components/component_sample_test.cpp  Sample user component runtime config source
class DynamicConfig final : public DynamicConfigUpdatesSinkBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of @ref components::DynamicConfig
    static constexpr std::string_view kName = "dynamic-config";

    using NoblockSubscriber = dynamic_config::NoblockSubscriber;

    DynamicConfig(const ComponentConfig&, const ComponentContext&);
    ~DynamicConfig() override;

    /// Use `dynamic_config::Source` to get up-to-date config values, or to do
    /// something special on config updates
    dynamic_config::Source GetSource();

    /// @brief Returns a constant @ref dynamic_config::Source built from this
    /// component's own fallback defaults (`dynamic-config.defaults`/
    /// `defaults-path`), without waiting for the first successful dynamic
    /// config update.
    ///
    /// Intended for bootstrapping components that must not have a blocking
    /// dependency on this component (e.g. to break a bootstrap cycle), while
    /// still needing a valid @ref dynamic_config::Source to pass around —
    /// for example, custom instances of components that are dependencies of
    /// the dynamic config updater itself. Values that matter for such a
    /// component should be set explicitly via `dynamic-config.defaults`/
    /// `defaults-path`, not left at their compile-time defaults.
    ///
    /// @warning The returned `Source` never updates at runtime. Prefer
    /// @ref GetSource() whenever possible.
    dynamic_config::Source GetDefaultsAsConstantSource();

    /// Get config defaults with overrides applied. Useful in the implementation
    /// of custom config clients. Most code does not need to deal with these
    const dynamic_config::DocsMap& GetDefaultDocsMap() const;

    /// @brief Returns metadata (name + schema_hash) of every dynamic config item
    /// registered in this binary at static-init time. ConstantConfig values and
    /// internal derived keys are not dynamic config items and are omitted. A
    /// dynamic_config::Key that parses multiple config items contributes metadata
    /// for each item.
    /// Repeated registrations of the same name are preserved, not deduplicated.
    /// schema_hash is an opaque value. It is empty if the registering
    /// constructor did not provide schema metadata.
    std::vector<dynamic_config::RegisteredConfigMeta> GetRegisteredConfigsMeta() const;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    friend class dynamic_config::NoblockSubscriber;

    ComponentHealth GetComponentHealth() const override;
    void OnLoadingCancelled() override;

    void SetConfig(std::string_view updater, dynamic_config::DocsMap&& value) override;

    void SetConfig(std::string_view updater, const dynamic_config::DocsMap& value) override;

    void NotifyLoadingFailed(std::string_view updater, std::string_view error) override;

    class Impl;
    std::unique_ptr<Impl> impl_;
};

template <>
inline constexpr bool kHasValidate<DynamicConfig> = true;

template <>
inline constexpr auto kConfigFileMode<DynamicConfig> = ConfigFileMode::kNotRequired;

dynamic_config::Source LocateDependency(
    const components::WithType<dynamic_config::Source>&,
    const components::ComponentConfig& config,
    const components::ComponentContext& context
);

}  // namespace components

USERVER_NAMESPACE_END
