#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/dynamic_config/client/component.hpp>
#include <userver/dynamic_config/updater/component.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/server/handlers/ping.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/server/handlers/tests_control.hpp>
#include <userver/server/http/http_response_body_stream.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/daemon_run.hpp>

class HandlerHttp2 final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-http2";

    HandlerHttp2(const components::ComponentConfig& config, const components::ComponentContext& context)
        : server::handlers::HttpHandlerBase(config, context) {}

    std::string HandleRequestThrow(const server::http::HttpRequest& req, server::request::RequestContext&)
        const override {
        const auto& type = req.GetArg("type");
        if (type == "echo-body") {
            return req.RequestBody();
        } else if (type == "echo-header") {
            return req.GetHeader("echo-header");
        } else if (type == "sleep") {
            engine::SleepFor(std::chrono::seconds{1});
        } else if (type == "json") {
            formats::json::Value json = formats::json::FromString(req.RequestBody());
            return ToString(json);
        }
        return "";
    };

    void HandleStreamRequest(
        const server::http::HttpRequest& req,
        server::request::RequestContext&,
        server::http::ResponseBodyStream& stream
    ) const override {
        const auto& type = req.GetArg("type");
        if (type == "eq") {
            const auto& body_part = req.GetArg("body_part");
            UASSERT(!body_part.empty());
            const auto& count_str = req.GetArg("count");
            const std::size_t count = std::stoi(count_str);
            UASSERT(count != 0);
            stream.SetStatusCode(200);
            stream.SetEndOfHeaders();
            for (std::size_t i = 0; i < count - 1; i++) {
                std::string part{body_part};
                stream.PushBodyChunk(std::move(part), {});
                // Some pause...
                engine::SleepFor(std::chrono::milliseconds{2});
            }
            std::string part{body_part};
            stream.PushBodyChunk(std::move(part), {});
        } else if (type == "ne") {
            stream.SetStatusCode(200);
            stream.SetEndOfHeaders();
            const auto& body = req.RequestBody();
            std::size_t pos = 0;
            std::size_t size = 1;
            while (pos < body.size() - 1) {
                stream.PushBodyChunk(body.substr(pos, size), {});  // 1, 2, 4, ... 512
                pos += size;
                size *= 2;
                engine::SleepFor(std::chrono::milliseconds{10});
            }
            stream.PushBodyChunk(body.substr(pos, 1), {});
        } else {
            throw std::runtime_error{"non-valid type"};
        }
    };
};

int main(int argc, char* argv[]) {
    auto component_list = components::MinimalServerComponentList()
                              .Append<components::TestsuiteSupport>()
                              .Append<components::HttpClient>()
                              .Append<clients::dns::Component>()
                              .Append<server::handlers::ServerMonitor>()
                              .Append<server::handlers::TestsControl>()
                              .Append<components::DynamicConfigClientUpdater>()
                              .Append<components::DynamicConfigClient>()
                              .Append<HandlerHttp2>()
                              .Append<server::handlers::Ping>();

    return utils::DaemonMain(argc, argv, component_list);
}
