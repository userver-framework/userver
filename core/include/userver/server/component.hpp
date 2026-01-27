#pragma once

/// @file userver/server/component.hpp
/// @brief @copybrief components::Server

#include <memory>

#include <userver/components/component_base.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/server/server.hpp>
#include <userver/utils/statistics/entry.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

/// @ingroup userver_components
///
/// @brief Component that listens for incoming requests, manages
/// incoming connections and passes the requests to the appropriate handler.
///
/// Starts listening and accepting connections only after **all** the
/// components are loaded.
///
/// All the classes inherited from server::handlers::HttpHandlerBase and
/// registered in components list bind to the components::Server component.
///
/// ## components::Server Dynamic config
/// * @ref USERVER_LOG_REQUEST
/// * @ref USERVER_LOG_REQUEST_HEADERS
/// * @ref USERVER_DEADLINE_PROPAGATION_ENABLED
/// * @ref USERVER_CANCEL_HANDLE_REQUEST_BY_DEADLINE
///
/// ## Static options of components::Server :
///
/// Server is configured by 'listener' and 'listener-monitor' entries.
/// 'listener' is a required entry that describes the request processing
/// socket. 'listener-monitor' is an optional entry that describes the special
/// monitoring socket, used for getting statistics and processing utility
/// requests that should succeed even is the main socket is under heavy pressure.
///
/// @include{doc} scripts/docs/en/components_schema/core/src/server/component.md
///
/// Options inherited from @ref components::RawComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// @see @ref scripts/docs/en/userver/http_server.md
class Server final : public ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of @ref components::Server component
    static constexpr std::string_view kName = "server";

    Server(const components::ComponentConfig& component_config, const components::ComponentContext& component_context);

    void OnAllComponentsLoaded() override;

    void OnAllComponentsAreStopping() override;

    const server::Server& GetServer() const;

    server::Server& GetServer();

    void AddHandler(const server::handlers::HttpHandlerBase& handler, engine::TaskProcessor& task_processor);

    static yaml_config::Schema GetStaticConfigSchema();

private:
    void WriteStatistics(utils::statistics::Writer& writer);

    std::unique_ptr<server::Server> server_;
};

template <>
inline constexpr bool kHasValidate<Server> = true;

}  // namespace components

USERVER_NAMESPACE_END
