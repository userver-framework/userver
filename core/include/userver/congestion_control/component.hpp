#pragma once

/// @file userver/congestion_control/component.hpp
/// @brief @copybrief congestion_control::Component

#include <userver/components/component_base.hpp>
#include <userver/dynamic_config/snapshot.hpp>
#include <userver/server/congestion_control/limiter.hpp>
#include <userver/server/congestion_control/sensor.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace congestion_control {

class Controller;

/// @ingroup userver_components
///
/// @brief Component to limit too active requests, also known as CC.
///
/// ## congestion_control::Component Dynamic config
/// * @ref USERVER_RPS_CCONTROL
/// * @ref USERVER_RPS_CCONTROL_ENABLED
///
/// ## Static options of congestion_control::Component :
/// @include{doc} scripts/docs/en/components_schema/core/src/congestion_control/component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// ## Static configuration example:
///
/// @snippet components/common_server_component_list_test.cpp  Sample congestion control component config
class Component final : public components::ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of @ref congestion_control::Component component
    static constexpr std::string_view kName = "congestion-control";

    Component(const components::ComponentConfig&, const components::ComponentContext&);

    ~Component() override;

    static yaml_config::Schema GetStaticConfigSchema();

    server::congestion_control::Limiter& GetServerLimiter();
    server::congestion_control::Sensor& GetServerSensor();
    const congestion_control::Controller& GetServerController() const;

private:
    void OnConfigUpdate(const dynamic_config::Snapshot& cfg);

    void OnAllComponentsLoaded() override;

    void OnAllComponentsAreStopping() override;

    void ExtendWriter(utils::statistics::Writer& writer);

    struct Impl;
    utils::FastPimpl<Impl, 704, 16> pimpl_;
};

}  // namespace congestion_control

template <>
inline constexpr bool components::kHasValidate<congestion_control::Component> = true;

USERVER_NAMESPACE_END
