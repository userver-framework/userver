#include <userver/server/component.hpp>

#include <server/server_config.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {
const storages::secdist::SecdistConfig& GetSecdist(
    const components::ComponentContext& component_context) {
  auto* component =
      component_context.FindComponentOptional<components::Secdist>();
  if (component) return component->Get();

  static storages::secdist::SecdistConfig kEmpty;
  return kEmpty;
}
}  // namespace

Server::Server(const components::ComponentConfig& component_config,
               const components::ComponentContext& component_context)
    : LoggableComponentBase(component_config, component_context),
      server_(std::make_unique<server::Server>(
          component_config.As<server::ServerConfig>(),
          GetSecdist(component_context), component_context)) {
  auto& statistics_storage =
      component_context.FindComponent<StatisticsStorage>().GetStorage();
  server_statistics_holder_ = statistics_storage.RegisterWriter(
      "server",
      [this](utils::statistics::Writer& writer) { WriteStatistics(writer); });
  handler_statistics_holder_ = statistics_storage.RegisterWriter(
      "http.handler.total", [this](utils::statistics::Writer& writer) {
        return server_->WriteTotalHandlerStatistics(writer);
      });
}

Server::~Server() {
  server_statistics_holder_.Unregister();
  handler_statistics_holder_.Unregister();
}

void Server::OnAllComponentsLoaded() { server_->Start(); }

void Server::OnAllComponentsAreStopping() {
  /* components::Server has to stop all Listeners before unloading components
   * as handlers have no ability to call smth like RemoveHandler() from
   * server::Server. Without such server stop before unloading a new request may
   * use a handler while the handler is destroying.
   */
  server_->Stop();
}

const server::Server& Server::GetServer() const { return *server_; }

server::Server& Server::GetServer() { return *server_; }

void Server::AddHandler(const server::handlers::HttpHandlerBase& handler,
                        engine::TaskProcessor& task_processor) {
  server_->AddHandler(handler, task_processor);
}

void Server::WriteStatistics(utils::statistics::Writer& writer) {
  server_->WriteMonitorData(writer);
}

yaml_config::Schema Server::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: server schema
additionalProperties: false
properties:
    logger_access:
        type: string
        description: set to logger name from components::Logging component to write access logs into it; do not set to avoid writing access logs
    logger_access_tskv:
        type: string
        description: set to logger name from components::Logging component to write access logs in TSKV format into it; do not set to avoid writing access logs
    max_response_size_in_flight:
        type: integer
        description: set it to the size of response in bytes and the component will drop bigger responses from handlers that allow throttling
    server-name:
        type: string
        description: value to send in HTTP Server header
        defaultDescription: value from utils::GetUserverIdentifier()
    listener:
        type: object
        description: describes the request processing socket
        additionalProperties: false
        properties: &server-listener-properties
            address:
                type: string
                description: IPv6 or IPv4 network interface to bind to
                defaultDescription: "::"
            port:
                type: integer
                description: port to listen on
                defaultDescription: 0
            unix-socket:
                type: string
                description: unix socket to listen on instead of listening on a port
                defaultDescription: ''
            max_connections:
                type: integer
                description: max connections count to keep
                defaultDescription: 32768
            task_processor:
                type: string
                description: task processor to process incoming requests
            backlog:
                type: integer
                description: max count of new connections pending acceptance
                defaultDescription: 1024
            tls:
                type: object
                description: TLS settings
                additionalProperties: false
                properties:
                    ca:
                        type: array
                        description: paths to TLS CAs
                        items:
                            type: string
                            description: path to TLS CA
                    cert:
                        type: string
                        description: path to TLS certificate
                    private-key:
                        type: string
                        description: path to TLS certificate private key
                    private-key-passphrase-name:
                        type: string
                        description: passphrase name located in secdist
            handler-defaults:
                type: object
                description: handler defaults options
                additionalProperties: false
                properties:
                    max_url_size:
                        type: integer
                        description: max path/URL size in bytes
                    max_request_size:
                        type: integer
                        description: max size of the whole data in bytes
                    max_headers_size:
                        type: integer
                        description: max headers size in bytes
                    parse_args_from_body:
                        type: boolean
                        description: optional field to parse request according to x-www-form-urlencoded rules and make parameters accessible as query parameters
                    set_tracing_headers:
                        type: boolean
                        description: whether to set http tracing headers (X-YaTraceId, X-YaSpanId, X-RequestId)
                        defaultDescription: true
                    deadline_propagation_enabled:
                        type: boolean
                        description: |
                            When `false`, disables deadline propagation by default in all HTTP handlers.
                            Can be overridden by the corresponding option in server::handlers::HandlerBase.
                        defaultDescription: true
                    deadline_expired_status_code:
                        type: integer
                        description: the HTTP status code to return if the request deadline expires
                        defaultDescription: 498
                        minimum: 400
                        maximum: 599
            connection:
                type: object
                description: connection options
                additionalProperties: false
                properties:
                    in_buffer_size:
                        type: integer
                        description: "size of the buffer to preallocate for request receive: bigger values use more RAM and less CPU"
                        defaultDescription: 32 * 1024
                    requests_queue_size_threshold:
                        type: integer
                        description: drop requests from handlers that allow throttling if there's more pending requests than allowed by this value
                        defaultDescription: 100
                    keepalive_timeout:
                        type: integer
                        description: timeout in seconds to drop connection if there's not data received from it
                        defaultDescription: 600
                    stream_close_check_delay:
                        type: integer
                        description: delay in microseconds of the start of abort check routine
                        defaultDescription: 20ms
            shards:
                type: integer
                description: how many concurrent tasks harvest data from a single socket; do not set if not sure what it is doing
    listener-monitor:
        type: object
        description: describes the special monitoring socket, used for getting statistics and processing utility requests that should succeed even is the main socket is under heavy pressure
        additionalProperties: false
        properties: *server-listener-properties
    set-response-server-hostname:
        type: boolean
        description: set to true to add the `X-YaTaxi-Server-Hostname` header with instance name, set to false to not add the header
        defaultDescription: false
    middleware-pipeline-builder:
        type: string
        description: name of a component to build a server-wide middleware pipeline
        defaultDescription: default-server-middleware-pipeline-builder
)");
}

}  // namespace components

USERVER_NAMESPACE_END
