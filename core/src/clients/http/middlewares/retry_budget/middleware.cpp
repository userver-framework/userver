#include <clients/http/middlewares/retry_budget/middleware.hpp>

#include <userver/clients/http/response.hpp>
#include <userver/http/url.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::middlewares::retry_budget {

USERVER_NAMESPACE::retry_budget::Storage& Middleware::Storage() { return storage_; }

void Middleware::HookCreateSpan(MiddlewareRequest& request, tracing::Span&) {
    // call GetDestination() to create RcuMap's key
    [[maybe_unused]] auto& _ = GetDestination(request.GetOriginalUrl());
}

void Middleware::HookOnCompleted(MiddlewareRequest& request, Response& response) {
    auto& dest = GetDestination(request.GetOriginalUrl());
    if (!response.IsError()) {
        dest.AccountOk();
    } else {
        dest.AccountFail();
    }
}

void Middleware::HookOnError(MiddlewareRequest& request, std::error_code) {
    auto& dest = GetDestination(request.GetOriginalUrl());
    dest.AccountFail();
}

bool Middleware::HookOnRetry(MiddlewareRequest& request) {
    auto& dest = GetDestination(request.GetOriginalUrl());
    dest.AccountFail();

    return dest.CanRetry();
}

USERVER_NAMESPACE::utils::RetryBudget& Middleware::GetDestination(const std::string& url) {
    return storage_.GetDestination(std::string{USERVER_NAMESPACE::http::ExtractHostname(url)});
}

}  // namespace clients::http::middlewares::retry_budget

USERVER_NAMESPACE_END
