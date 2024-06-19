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

// clang-format off

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
/// ## Dynamic config
/// * @ref USERVER_LOG_REQUEST
/// * @ref USERVER_LOG_REQUEST_HEADERS
/// * @ref USERVER_DEADLINE_PROPAGATION_ENABLED
/// * @ref USERVER_CANCEL_HANDLE_REQUEST_BY_DEADLINE
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// logger_access | set to logger name from components::Logging component to write access logs into it; do not set to avoid writing access logs  | -
/// logger_access_tskv | set to logger name from components::Logging component to write access logs in TSKV format into it; do not set to avoid writing access logs | -
/// max_response_size_in_flight | set it to the size of response in bytes and the component will drop bigger responses from handlers that allow throttling | -
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
/// address | IPv6 or IPv4 network interface to bind to | '::' (all the IPv6 and IPv4 local interfaces)
/// port | port to listen on | 0
/// unix-socket | unix socket to listen on instead of listening on a port and network address | ''
/// max_connections | max connections count to keep | 32768
/// task_processor | task processor to process incoming requests | -
/// backlog | max count of new connections pending acceptance | 1024
/// tls.ca | paths to TLS CAs for client authentication | -
/// tls.cert | path to TLS server certificate | -
/// tls.private-key | path to TLS server certificate private key | -
/// tls.private-key-passphrase-name | passphrase name located in secdist's "passphrases" section | -
/// handler-defaults.max_url_size | max path/URL size or empty to not limit | 8192
/// handler-defaults.max_request_size | max size of the whole request | 1024 * 1024
/// handler-defaults.max_headers_size | max request headers size | 65536
/// handler-defaults.parse_args_from_body | optional field to parse request according to x-www-form-urlencoded rules and make parameters accessible as query parameters | false
/// handler-defaults.set_tracing_headers | whether to set http tracing headers (X-YaTraceId, X-YaSpanId, X-RequestId) | true
/// handler-defaults.deadline_propagation_enabled | when `false`, disables HTTP handler deadline propagation | true
/// handler-defaults.deadline_expired_status_code | the HTTP status code to return if the request deadline expires | 498
/// connection.in_buffer_size | size of the buffer to preallocate for request receive: bigger values use more RAM and less CPU | 32 * 1024
/// connection.requests_queue_size_threshold | drop requests from handlers that allow throttling if there's more pending requests than allowed by this value | 100
/// connection.keepalive_timeout | timeout in seconds to drop connection if there's not data received from it | 600
/// connection.stream_close_check_delay | delay in microseconds of the start of stream close check routine; do not set if not sure what it is doing | 20ms
/// shards | how many concurrent tasks harvest data from a single socket; do not set if not sure what it is doing | -
/// middleware-pipeline-builder | name of a component to build a server-wide middleware pipeline | default-server-middleware-pipeline-builder
///
/// @see @ref scripts/docs/en/userver/http_server.md

// clang-format on

class Server final : public ComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of components::Server component
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
  void WriteStatistics(utils::statistics::Writer& writer);

  std::unique_ptr<server::Server> server_;
  utils::statistics::Entry server_statistics_holder_;
  utils::statistics::Entry handler_statistics_holder_;
};

template <>
inline constexpr bool kHasValidate<Server> = true;

}  // namespace components

USERVER_NAMESPACE_END
