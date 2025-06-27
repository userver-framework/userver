#pragma once

#include <userver/clients/http/plugin.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::plugins::yandex_tracing {

class Plugin final : public http::Plugin {
public:
    Plugin();

    void HookPerformRequest(PluginRequest& request) override;

    void HookCreateSpan(PluginRequest& request, tracing::Span& span) override;

    void HookOnCompleted(PluginRequest& request, Response& response) override;

    void HookOnError(PluginRequest& request, std::error_code ec) override;

    bool HookOnRetry(PluginRequest& request) override;
};

}  // namespace clients::http::plugins::yandex_tracing

USERVER_NAMESPACE_END
