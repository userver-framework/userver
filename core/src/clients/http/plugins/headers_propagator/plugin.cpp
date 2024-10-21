#include <clients/http/plugins/headers_propagator/plugin.hpp>

#include <userver/http/url.hpp>
#include <userver/server/request/task_inherited_request.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::plugins::headers_propagator {

namespace {
const std::string kName = "headers-propagator";
}  // namespace

Plugin::Plugin() : clients::http::Plugin(kName) {}

void Plugin::HookPerformRequest(PluginRequest&) {}

void Plugin::HookCreateSpan(PluginRequest& request) {
    for (const auto& [name, value] : server::request::GetPropagatedHeaders()) {
        request.SetHeader(name, value);
    }
}

void Plugin::HookOnCompleted(PluginRequest&, Response&) {}

}  // namespace clients::http::plugins::headers_propagator

USERVER_NAMESPACE_END
