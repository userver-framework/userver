#include <userver/server/handlers/websocket_handler.hpp>

#include <cryptopp/sha.h>

#include <userver/components/statistics_storage.hpp>
#include <userver/crypto/base64.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/str_icase.hpp>
#include <userver/websocket/connection.hpp>
#include <userver/websocket/impl/protocol.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/server/handlers/websocket_handler.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace server::handlers {

namespace {

/// @brief Checks the `Sec-WebSocket-Key` of the RFC 6455 handshake over HTTP/1.1.
/// The extended CONNECT of RFC 8441 has no counterpart: opening the stream is itself
/// the proof of intent.
const std::string& GetCheckedWebsocketKey(const server::http::HttpRequest& request) {
    const std::string& sec_websocket_key = request.GetHeader(USERVER_NAMESPACE::http::headers::kWebsocketKey);

    // We are fine if `secWebsocketKey` is not properly base64-ecoded
    static constexpr std::size_t kLengthOfBase64Encoded16Bytes = 24;
    if (kLengthOfBase64Encoded16Bytes != sec_websocket_key.size()) {
        LOG_WARNING() << "Empty or invalid Websocket Key";
        throw server::handlers::ClientError();
    }
    return sec_websocket_key;
}

}  // namespace

WebsocketHandlerBase::WebsocketHandlerBase(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : server::handlers::HttpHandlerBase(config, context),
      config_(config.As<websocket::Config>())
{
    utils::statistics::RegisterWriterScope(context, "ws." + config.Name(), [this](utils::statistics::Writer& writer) {
        WriteMetrics(writer);
    });
}

bool WebsocketHandlerBase::IsWebsocketRequest(const server::http::HttpRequest& request) const {
    if (request.IsWebsocketExtendedConnect()) {
        // RFC 8441 carries no Upgrade/Connection headers, and the extended CONNECT form
        // has already been validated while parsing the HTTP/2.0 stream.
        return true;
    }

    constexpr auto kIcaseEq = utils::StrIcaseEqual();

    return request.GetMethod() == server::http::HttpMethod::kGet &&
           kIcaseEq(request.GetHeader(USERVER_NAMESPACE::http::headers::kUpgrade), "websocket") &&
           kIcaseEq(request.GetHeader(USERVER_NAMESPACE::http::headers::kConnection), "upgrade");
}

void WebsocketHandlerBase::HandleWebsocketRequest(
    server::http::HttpRequest& request,
    server::request::RequestContext& context
) const {
    const bool is_extended_connect = request.IsWebsocketExtendedConnect();
    // Checked before anything else, as an invalid key is a bad request no matter what
    // websocket version was asked for.
    const std::string sec_websocket_key = is_extended_connect ? std::string{} : GetCheckedWebsocketKey(request);

    auto& response = request.GetHttpResponse();

    const auto& version = request.GetHeader(USERVER_NAMESPACE::http::headers::kWebsocketVersion);
    if (version != "13") {
        LOG_WARNING() << "Wrong websocket version: " << version;
        response.SetHeader(USERVER_NAMESPACE::http::headers::kWebsocketVersion, "13");
        response.SetStatus(server::http::HttpStatus::kUpgradeRequired);
        return;
    }

    if (!HandleHandshake(request, context)) {
        return;
    }

    if (is_extended_connect) {
        // RFC 8441: the stream is accepted with a plain 200 and stays open. There is no
        // protocol switch to announce, as the connection keeps speaking HTTP/2.0.
        response.SetStatus(server::http::HttpStatus::kOk);
    } else {
        response.SetStatus(server::http::HttpStatus::kSwitchingProtocols);
        response.SetHeader(USERVER_NAMESPACE::http::headers::kConnection, "Upgrade");
        response.SetHeader(USERVER_NAMESPACE::http::headers::kUpgrade, "websocket");
        response.SetHeader(
            USERVER_NAMESPACE::http::headers::kWebsocketAccept,
            websocket::impl::WebsocketSecAnswer(sec_websocket_key)
        );
    }

    request.SetUpgradeWebsocket([context = std::make_shared<server::request::RequestContext>(std::move(context)),
                                 this](std::unique_ptr<engine::io::RwBase> socket, engine::io::Sockaddr&& peer_name) {
        tracing::Span span("ws/" + HandlerName());
        auto ws = websocket::MakeServerWebSocketConnection(std::move(socket), std::move(peer_name), config_);
        try {
            Handle(*ws, *context);
        } catch (const std::exception& e) {
            LOG_WARNING() << "Unhandled exception in ws handler: " << e;
        }

        ws->AddFinalTags(span);
        ws->AddStatistics(stats_);
    });
}

std::string
WebsocketHandlerBase::HandleNonWebsocketRequest(server::http::HttpRequest&, server::request::RequestContext&) const {
    LOG_WARNING() << "Not a GET 'Upgrade: websocket' and 'Connection: Upgrade' request";
    throw server::handlers::ClientError();
}

std::string WebsocketHandlerBase::HandleRequest(
    server::http::HttpRequest& request,
    server::request::RequestContext& context
) const {
    if (!IsWebsocketRequest(request)) {
        return HandleNonWebsocketRequest(request, context);
    }

    HandleWebsocketRequest(request, context);
    return "";
}

void WebsocketHandlerBase::WriteMetrics(utils::statistics::Writer& writer) const {
    writer["msg"]["sent"] = stats_.msg_sent;
    writer["msg"]["recv"] = stats_.msg_recv;

    writer["bytes"]["sent"] = stats_.bytes_sent;
    writer["bytes"]["recv"] = stats_.bytes_recv;
}

yaml_config::Schema WebsocketHandlerBase::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<
        server::handlers::HttpHandlerBase>("src/server/handlers/websocket_handler.yaml");
}

}  // namespace server::handlers

USERVER_NAMESPACE_END
