#include "http_server_logs.hpp"

#include <gtest/gtest.h>

#include <string>
#include <string_view>

#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/server/handlers/handler_config.hpp>
#include <userver/server/handlers/json_body_logging.hpp>
#include <userver/utils/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::impl {
namespace {

TEST(HttpServerLogs, IndependentPoliciesAndLimits) {
    dynamic_config::DocsMap docs;
    docs.Set("USERVER_HTTP_SERVER_LOGS", formats::json::FromString(R"({
        "/echo": {
            "request_body_size_log_limit":100,
            "json_request_body_masking":{"paths":[["token"]]},
            "json_response_body_masking":{"list_mode":"whitelist","paths":[["token"]]}
        },
        "handler-override": {
            "request_body_size_log_limit":100,
            "response_body_size_log_limit":100,
            "json_request_body_masking":{"paths":[["id"]]},
            "json_response_body_masking":{"list_mode":"whitelist","paths":[["id"]]}
        },
        "limit-only-override": {
            "request_body_size_log_limit":1
        },
        "/unconfigured": {}
    })"));
    dynamic_config::StorageMock storage{{kHttpServerLogs, kHttpServerLogs.Parse(docs)}};
    const auto snapshot = storage.GetSnapshot();
    HandlerConfig config;
    config.path = "/echo";
    config.request_body_size_log_limit = 1;
    config.response_data_size_log_limit = 100;
    const std::string body = R"({"id":1,"token":2})";
    EXPECT_EQ(MaskJsonRequestBodyForLogging(body, snapshot, config), R"({"id":1,"token":"..."})");
    EXPECT_EQ(MaskJsonResponseBodyForLogging(body, snapshot, config), R"({"id":"...","token":2})");
    EXPECT_EQ(MaskJsonRequestBodyForLogging(std::string{body}, snapshot, config), R"({"id":1,"token":"..."})");
    EXPECT_EQ(MaskJsonResponseBodyForLogging(std::string{body}, snapshot, config), R"({"id":"...","token":2})");

    constexpr std::string_view kHandlerName{"handler-override"};
    constexpr JsonBodyLoggingOptions kOptions{.handler_name_override = kHandlerName};
    EXPECT_EQ(MaskJsonRequestBodyForLogging(body, snapshot, config, kOptions), R"({"id":"...","token":2})");
    EXPECT_EQ(MaskJsonResponseBodyForLogging(body, snapshot, config, kOptions), R"({"id":1,"token":"..."})");
    EXPECT_EQ(
        MaskJsonRequestBodyForLogging(std::string{body}, snapshot, config, kOptions),
        R"({"id":"...","token":2})"
    );
    EXPECT_EQ(
        MaskJsonResponseBodyForLogging(std::string{body}, snapshot, config, kOptions),
        R"({"id":1,"token":"..."})"
    );
    EXPECT_EQ(
        MaskJsonRequestBodyForLogging(body, snapshot, config, {.handler_name_override = "missing-handler"}),
        body
    );
    EXPECT_EQ(
        MaskJsonRequestBodyForLogging(body, snapshot, config, {.handler_name_override = "limit-only-override"}),
        body
    );

    config.response_data_size_log_limit = 1;
    EXPECT_EQ(MaskJsonResponseBodyForLogging(body, snapshot, config), ".");
    for (const auto* path : {"/missing", "/unconfigured"}) {
        config.path = path;
        EXPECT_EQ(MaskJsonRequestBodyForLogging(body, snapshot, config), body);
        EXPECT_EQ(MaskJsonResponseBodyForLogging(body, snapshot, config), body);
        EXPECT_EQ(MaskJsonRequestBodyForLogging(std::string{body}, snapshot, config), body);
        EXPECT_EQ(MaskJsonResponseBodyForLogging(std::string{body}, snapshot, config), body);
    }
}

TEST(HttpServerLogs, TruncationFitsLogLimit) {
    const std::string_view body =
        R"({"secret":"sensitive","text":")"
        "\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2"
        "\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2"
        "\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2"
        "\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2\u00a2"
        R"("})";
    const formats::json::MaskingRules rules{{{"secret"}}};
    const std::string_view suffix = "...(truncated, total 112 bytes)";

    for (const std::size_t limit : {0, 1, 30, 31, 32, 80, 81, 100}) {
        SCOPED_TRACE(limit);
        const auto result = MaskBodyForLogging(body, rules, limit);
        ASSERT_LE(result.size(), limit);
        if (limit < suffix.size()) {
            EXPECT_EQ(result, suffix.substr(0, limit));
        } else {
            ASSERT_GE(result.size(), suffix.size());
            EXPECT_EQ(std::string_view{result}.substr(result.size() - suffix.size()), suffix);
        }
        EXPECT_EQ(utils::log::ToLimitedUtf8(std::string{result}, limit), result);
    }
}

TEST(HttpServerLogs, InvalidAndEmptyBodies) {
    const formats::json::MaskingRules rules{{{"token"}}};
    EXPECT_EQ(MaskBodyForLogging(R"({"token":SECRET})", rules, 100), "<error masking JSON body>");
    EXPECT_EQ(MaskBodyForLogging("", rules, 100), "");
}

}  // namespace
}  // namespace server::handlers::impl

USERVER_NAMESPACE_END
