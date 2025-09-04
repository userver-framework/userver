#pragma once

#include <clients/http/plugins/retry_budget/storage.hpp>
#include <userver/clients/http/plugin.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::plugins::retry_budget {

class Plugin final : public http::Plugin {
public:
    Plugin();

    USERVER_NAMESPACE::retry_budget::Storage& Storage();

    void HookPerformRequest(PluginRequest& request) override;

    void HookCreateSpan(PluginRequest& request, tracing::Span& span) override;

    void HookOnCompleted(PluginRequest& request, Response& response) override;

    void HookOnError(PluginRequest& request, std::error_code ec) override;

    bool HookOnRetry(PluginRequest& request) override;

private:
    USERVER_NAMESPACE::utils::RetryBudget& GetDestination(const std::string& url);

    USERVER_NAMESPACE::retry_budget::Storage storage_;
};

}  // namespace clients::http::plugins::retry_budget

USERVER_NAMESPACE_END
