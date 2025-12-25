#pragma once

#include <clients/http/middlewares/retry_budget/storage.hpp>
#include <userver/clients/http/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::middlewares::retry_budget {

class Middleware final : public http::MiddlewareBase {
public:
    USERVER_NAMESPACE::retry_budget::Storage& Storage();

    void HookCreateSpan(MiddlewareRequest& request, tracing::Span& span) override;

    void HookOnCompleted(MiddlewareRequest& request, Response& response) override;

    void HookOnError(MiddlewareRequest& request, std::error_code ec) override;

    bool HookOnRetry(MiddlewareRequest& request) override;

private:
    USERVER_NAMESPACE::utils::RetryBudget& GetDestination(const std::string& url);

    USERVER_NAMESPACE::retry_budget::Storage storage_;
};

}  // namespace clients::http::middlewares::retry_budget

USERVER_NAMESPACE_END
