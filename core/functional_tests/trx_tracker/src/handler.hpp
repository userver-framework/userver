#pragma once

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/trx_tracker.hpp>

namespace handlers {

class Handler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler";

    Handler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context)
    {}

    std::string HandleRequestThrow(const server::http::HttpRequest& /*request*/, server::request::RequestContext&)
        const override {
        utils::trx_tracker::TransactionLock lock;
        lock.Lock();
        // Tespoint shouldn't actually trigger heavy operation
        // in transaction check despite being implemented
        // using http request
        TESTPOINT("tp", formats::json::Value{});
        // Other heavy operation in transaction check
        // should actually trigger
        utils::trx_tracker::CheckNoTransactions();
        return "";
    }
};

}  // namespace handlers
