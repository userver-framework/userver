#pragma once

/// @file userver/server/component.hpp
/// @brief @copybrief components::Server

#include <memory>

#include <userver/components/loggable_component_base.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/server/server.hpp>
#include <userver/yaml_config/schema.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

class StatisticsStorage;

// clang-format off

/// @ingroup userver_components
///
/// @brief Component that listens for incomming requests, manages
/// incomming connections and passes the requests to the appropriate handler.
///
/// Starts listening and accepting connections only after **all** the
/// components are loaded.
///
/// ## Dynamic config
/// * @ref USERVER_LOG_REQUEST
/// * @ref USERVER_LOG_REQUEST_HEADERS
/// * @ref USERVER_CHECK_AUTH_IN_HANDLERS
/// * @ref USERVER_CANCEL_HANDLE_REQUEST_BY_DEADLINE
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// logger_access | set to logger name from components::Logging component to write access logs into it; do not set to avoid writing access logs  | -
/// logger_access_tskv | set to logger name from components::Logging component to write access logs in TSKV format into it; do not set to avoid writing access logs | -
/// max_response_size_in_flight | set it to the size of response in bytes and the component will drop bigger responses from handlers that allow trottling | -
/// server-name | value to send in HTTP Server header | value from utils::GetUserverIdentifier()
/// listener | (*required*) *see below* | -
/// listener-monitor | *see below* | -
/// set-response-server-hostname | set to true to add the `X-YaTaxi-Server-Hostname` header with instance name, set to false to not add the header | false
///
/// Server is configured by 'listener' and 'listener-monitor' entries.
/// 'listener' is a required entry that describes the request processing
/// socket. 'listener-monitor' is an optional entry that describes the special
/// monitoring socket, used for getting statistics and processing utility
/// requests that should succeed even is the main socket is under heavy
/// pressure.
///
/// Each of the 'listener' and 'listener-monitor' may be configured with the
/// following options:
///
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// port | port to listen on | 0
/// unix-socket | unix socket to listen on instead of listening on a port | ''
/// max_connections | max connections count to keep | 32768
/// task_processor | task processor to process incomming requests | -
/// backlog | max count of new coneections pending acceptance | 1024
/// connection.in_buffer_size | size of the buffer to preallocate for request receive: bigger values use more RAM and less CPU | 32 * 1024
/// connection.requests_queue_size_threshold | drop requests from handlers that allow trottling if there's more pending requests than allowed by this value | 100
/// connection.keepalive_timeout | timeout in seconds to drop connection if there's not data received from it | 600
/// connection.request.type | type of the request, only 'http' supported at the moment | 'http'
/// shards | how many concurrent tasks harvest data from a single socket; do not set if not sure what it is doing | -

// clang-format on

class Server final : public LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "server";

  Server(const components::ComponentConfig& component_config,
         const components::ComponentContext& component_context);

  ~Server() override;

  void OnAllComponentsLoaded() override;

  void OnAllComponentsAreStopping() override;

  const server::Server& GetServer() const;

  server::Server& GetServer();

  void AddHandler(const server::handlers::HttpHandlerBase& handler,
                  engine::TaskProcessor& task_processor);

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  formats::json::Value ExtendStatistics(
      const utils::statistics::StatisticsRequest& /*request*/);

 private:
  std::unique_ptr<server::Server> server_;
  StatisticsStorage& statistics_storage_;
  utils::statistics::Entry statistics_holder_;
};

template <>
inline constexpr bool kHasValidate<Server> = true;

}  // namespace components

USERVER_NAMESPACE_END
