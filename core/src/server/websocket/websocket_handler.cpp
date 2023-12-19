#include <userver/server/websocket/websocket_handler.hpp>

#include <cryptopp/sha.h>

#include <userver/components/statistics_storage.hpp>
#include <userver/crypto/base64.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/utils/async.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/server/websocket/server.hpp>
#include "protocol.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::websocket {

WebsocketHandlerBase::WebsocketHandlerBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      config_(config.As<Config>()) {
  auto& statistics_storage =
      context.FindComponent<components::StatisticsStorage>().GetStorage();
  statistics_holder_ = statistics_storage.RegisterWriter(
      "ws." + config.Name(), [this](utils::statistics::Writer& writer) {
        return WriteMetrics(writer);
      });
}

std::string WebsocketHandlerBase::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext& context) const {
  if (request.GetMethod() != server::http::HttpMethod::kGet ||
      request.GetHeader(USERVER_NAMESPACE::http::headers::kUpgrade) !=
          "websocket" ||
      request.GetHeader(USERVER_NAMESPACE::http::headers::kConnection) !=
          "Upgrade") {
    LOG_WARNING()
        << "Not a GET 'Upgrade: websocket' and 'Connection: Upgrade' request";
    throw server::handlers::ClientError();
  }

  const std::string& secWebsocketKey =
      request.GetHeader(USERVER_NAMESPACE::http::headers::kWebsocketKey);
  if (secWebsocketKey.empty()) {
    LOG_WARNING() << "Empty or missing Websocket Key";
    throw server::handlers::ClientError();
  }

  auto& response = request.GetHttpResponse();

  const auto& version =
      request.GetHeader(USERVER_NAMESPACE::http::headers::kWebsocketVersion);
  if (version != "13") {
    LOG_WARNING() << "Wrong websocket version: " << version;
    response.SetHeader(USERVER_NAMESPACE::http::headers::kWebsocketVersion,
                       "13");
    response.SetStatus(server::http::HttpStatus::kUpgradeRequired);
    return "";
  }

  if (!HandleHandshake(request, response, context)) return "";

  response.SetStatus(server::http::HttpStatus::kSwitchingProtocols);
  response.SetHeader(USERVER_NAMESPACE::http::headers::kConnection, "Upgrade");
  response.SetHeader(USERVER_NAMESPACE::http::headers::kUpgrade, "websocket");
  response.SetHeader(USERVER_NAMESPACE::http::headers::kWebsocketAccept,
                     websocket::impl::WebsocketSecAnswer(secWebsocketKey));

  request.SetUpgradeWebsocket(
      [context = std::make_shared<server::request::RequestContext>(
           std::move(context)),
       this](std::unique_ptr<engine::io::RwBase> socket,
             engine::io::Sockaddr&& peer_name) {
        tracing::Span span("ws/" + HandlerName());
        auto ws = websocket::MakeWebSocket(std::move(socket),
                                           std::move(peer_name), config_);
        try {
          Handle(*ws, *context);
        } catch (const std::exception& e) {
          LOG_WARNING() << "Unhandled exception in ws handler: " << e;
        }

        ws->AddFinalTags(span);
        ws->AddStatistics(stats_);
      });
  return "";
}

void WebsocketHandlerBase::WriteMetrics(
    utils::statistics::Writer& writer) const {
  writer["msg"]["sent"] = stats_.msg_sent.load();
  writer["msg"]["recv"] = stats_.msg_recv.load();

  writer["bytes"]["sent"] = stats_.bytes_sent.load();
  writer["bytes"]["recv"] = stats_.bytes_recv.load();
}

yaml_config::Schema WebsocketHandlerBase::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<server::handlers::HttpHandlerBase>(R"(
type: object
description: Base class for WebSocket handlers
additionalProperties: false
properties:
    max-remote-payload:
        type: integer
        description: max input message payload size
        defaultDescription: 65536
    fragment-size:
        type: integer
        description: max output fragment size
        defaultDescription: 65536
)");
}

}  // namespace server::websocket

USERVER_NAMESPACE_END
