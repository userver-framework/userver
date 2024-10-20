#pragma once

/// @file userver/components/raw_component_base.hpp
/// @brief @copybrief components::RawComponentBase

USERVER_NAMESPACE_BEGIN

namespace yaml_config {

struct Schema;

}  // namespace yaml_config

namespace components {

/// State of the component
enum class ComponentHealth {
    /// component is alive and fine
    kOk,
    /// component in fallback state, but the service keeps working
    kFallback,
    /// component is sick, service can not work without it
    kFatal,
};

/// Whether the static config for the component must always be present, or can
/// be missing
enum class ConfigFileMode {
    /// component must be setup in config
    kRequired,
    /// component must be not setup in config
    kNotRequired
};

/// @brief The base class for all components. Don't use it for application
/// components, use ComponentBase instead.
///
/// Only inherited directly by some of the built-in userver components.
class RawComponentBase {
public:
    RawComponentBase() = default;

    RawComponentBase(RawComponentBase&&) = delete;

    RawComponentBase(const RawComponentBase&) = delete;

    virtual ~RawComponentBase();

    virtual ComponentHealth GetComponentHealth() const { return ComponentHealth::kOk; }

    virtual void OnLoadingCancelled() {}

    virtual void OnAllComponentsLoaded() {}

    virtual void OnAllComponentsAreStopping() {}

    static yaml_config::Schema GetStaticConfigSchema();
};

/// Specialize it for typename Component to validate static config against
/// schema from Component::GetStaticConfigSchema
///
/// @see @ref static-configs-validation "Static configs validation"
template <typename Component>
inline constexpr bool kHasValidate = false;

/// Specialize this to customize the loading of component settings
///
/// @see @ref select-config-file-mode "Setup config file mode"
template <typename Component>
inline constexpr auto kConfigFileMode = ConfigFileMode::kRequired;

}  // namespace components

USERVER_NAMESPACE_END
