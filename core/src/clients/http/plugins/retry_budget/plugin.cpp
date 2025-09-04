#include <clients/http/plugins/retry_budget/plugin.hpp>

#include <userver/clients/http/response.hpp>
#include <userver/http/url.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::plugins::retry_budget {

Plugin::Plugin() : http::Plugin("retry-budget") {}

USERVER_NAMESPACE::retry_budget::Storage& Plugin::Storage() { return storage_; }

void Plugin::HookPerformRequest(PluginRequest&) {}

void Plugin::HookCreateSpan(PluginRequest& request, tracing::Span&) {
    // call GetDestination() to create RcuMap's key
    [[maybe_unused]] auto& _ = GetDestination(request.GetOriginalUrl());
}

void Plugin::HookOnCompleted(PluginRequest& request, Response& response) {
    auto& dest = GetDestination(request.GetOriginalUrl());
    if (!response.IsError())
        dest.AccountOk();
    else
        dest.AccountFail();
}

void Plugin::HookOnError(PluginRequest& request, std::error_code) {
    auto& dest = GetDestination(request.GetOriginalUrl());
    dest.AccountFail();
}

bool Plugin::HookOnRetry(PluginRequest& request) {
    auto& dest = GetDestination(request.GetOriginalUrl());
    dest.AccountFail();

    return dest.CanRetry();
}

USERVER_NAMESPACE::utils::RetryBudget& Plugin::GetDestination(const std::string& url) {
    return storage_.GetDestination(std::string{USERVER_NAMESPACE::http::ExtractHostname(url)});
}

}  // namespace clients::http::plugins::retry_budget

USERVER_NAMESPACE_END
