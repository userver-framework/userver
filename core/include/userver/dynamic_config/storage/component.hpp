#pragma once

/// @file userver/dynamic_config/storage/component.hpp
/// @brief @copybrief components::DynamicConfig

#include <memory>
#include <string_view>
#include <type_traits>

#include <userver/components/component_base.hpp>
#include <userver/concurrent/async_event_source.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/dynamic_config/updates_sink/component.hpp>
#include <userver/utils/fast_pimpl.hpp>

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
/// @snippet components/common_component_list_test.cpp  Sample dynamic config component config
///
/// ## Usage example:
/// @snippet components/component_sample_test.cpp  Sample user component runtime config source
class DynamicConfig final : public DynamicConfigUpdatesSinkBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of @ref components::DynamicConfig
    static constexpr std::string_view kName = "dynamic-config";

    class NoblockSubscriber;

    DynamicConfig(const ComponentConfig&, const ComponentContext&);
    ~DynamicConfig() override;

    /// Use `dynamic_config::Source` to get up-to-date config values, or to do
    /// something special on config updates
    dynamic_config::Source GetSource();

    /// Get config defaults with overrides applied. Useful in the implementation
    /// of custom config clients. Most code does not need to deal with these
    const dynamic_config::DocsMap& GetDefaultDocsMap() const;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    ComponentHealth GetComponentHealth() const override;
    void OnLoadingCancelled() override;

    void SetConfig(std::string_view updater, dynamic_config::DocsMap&& value) override;

    void SetConfig(std::string_view updater, const dynamic_config::DocsMap& value) override;

    void NotifyLoadingFailed(std::string_view updater, std::string_view error) override;

    class Impl;
    std::unique_ptr<Impl> impl_;
};

/// @brief Allows to subscribe to `DynamicConfig` updates without waiting for
/// the first update to complete. Primarily intended for internal use.
class DynamicConfig::NoblockSubscriber final {
public:
    explicit NoblockSubscriber(DynamicConfig& config_component) noexcept;

    NoblockSubscriber(NoblockSubscriber&&) = delete;
    NoblockSubscriber& operator=(NoblockSubscriber&&) = delete;

    /// @brief Subscribes to dynamic-config updates with information about the
    /// current and previous states.
    ///
    /// Subscribes to dynamic-config updates using a member function, named
    /// `OnConfigUpdate` by convention. actual configs values are already loaded, also constructs @ref
    /// dynamic_config::Diff object using `std::nullopt` and current config snapshot, then immediately invokes the
    /// function with it (this invocation will be executed synchronously).
    ///
    /// @note Callbacks occur in full accordance with
    /// @ref components::DynamicConfigClientUpdater options.
    ///
    /// @warning In debug mode the last notification for any subscriber will be
    /// called with `std::nullopt` and current config snapshot.
    ///
    /// Example usage:
    /// @snippet dynamic_config/config_test.cpp Custom subscription for dynamic config update
    ///
    /// @param obj the subscriber, which is the owner of the listener method, and
    /// is also used as the unique identifier of the subscription
    /// @param name the name of the subscriber, for diagnostic purposes
    /// @param func the listener method, named `OnConfigUpdate` by convention.
    /// @returns a @ref concurrent::AsyncEventSubscriberScope controlling the
    /// subscription, which should be stored as a member in the subscriber;
    /// `Unsubscribe` should be called explicitly
    ///
    /// @see based on @ref concurrent::AsyncEventSource engine
    ///
    /// @see dynamic_config::Diff
    template <typename Class>
    concurrent::AsyncEventSubscriberScope UpdateIfHasConfigAndListen(
        Class* obj,
        std::string_view name,
        void (Class::*func)(const dynamic_config::Diff& diff)
    ) {
        return DoUpdateIfHasConfigAndListen(
            concurrent::FunctionId(obj),
            name,
            [obj, func](const dynamic_config::Diff& diff) { (obj->*func)(diff); }
        );
    }

private:
    concurrent::AsyncEventSubscriberScope DoUpdateIfHasConfigAndListen(
        concurrent::FunctionId id,
        std::string_view name,
        concurrent::AsyncEventSource<const dynamic_config::Diff&>::Function&& func
    );

    DynamicConfig& config_component_;
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
