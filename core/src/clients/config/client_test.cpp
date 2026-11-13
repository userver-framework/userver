#include <userver/dynamic_config/client/client.hpp>

#include <atomic>
#include <chrono>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

#include <userver/clients/http/client.hpp>
#include <userver/clients/http/error.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

namespace {

constexpr int kStatusOk = 200;
constexpr int kStatusBadRequest = 400;
constexpr int kStatusInternalServerError = 500;
constexpr std::string_view kConfigsValuesPath = "/configs/values";

class ConfigsServerMock final {
public:
    ConfigsServerMock(
        int response_status,
        std::string updated_at,
        std::string_view expected_body = R"({"service":"service"})"
    )
        : server_([this,
                   response_status,
                   expected_body = formats::json::FromString(expected_body),
                   updated_at = std::move(updated_at)](const utest::HttpServerMock::HttpRequest& request) {
              ++requests_count_;
              EXPECT_EQ(request.path, kConfigsValuesPath);
              EXPECT_EQ(formats::json::FromString(request.body), expected_body);
              return utest::HttpServerMock::HttpResponse{
                  response_status,
                  {},
                  fmt::format(R"({{"configs": {{}}, "updated_at": "{}"}})", updated_at),
              };
          })
    {}

    std::string GetBaseUrl() const { return server_.GetBaseUrl(); }

    int GetRequestsCount() const { return requests_count_.load(); }

private:
    std::atomic<int> requests_count_{0};
    const utest::HttpServerMock server_;
};

ClientConfig MakeClientConfig(std::string config_url, std::vector<std::string> fallback_config_urls) {
    ClientConfig config;
    config.service_name = "service";
    config.timeout = std::chrono::duration_cast<std::chrono::milliseconds>(utest::kMaxTestWaitTime);
    config.retries = 1;
    config.config_url = std::move(config_url);
    config.fallback_config_urls = std::move(fallback_config_urls);
    return config;
}

}  // namespace

UTEST_DEATH(ClientConfigDeathTest, ServiceAndServiceOverridesAreMutuallyExclusive) {
    auto http_client = utest::CreateHttpClient();

    ClientConfig config;
    config.service_name = "host-service";
    config.service_overrides = {"host-service:sidecar", "host-service"};

    EXPECT_UINVARIANT_FAILURE_MSG(
        (Client{*http_client, config}),
        "dynamic_config::ClientConfig cannot use 'service' and 'service_overrides' together"
    );
}

UTEST(DynamicConfigClient, UsesConfigUrlWhenItSucceeds) {
    const ConfigsServerMock primary{kStatusOk, "primary"};
    const ConfigsServerMock fallback{kStatusOk, "fallback"};
    auto http_client = utest::CreateHttpClient();
    Client client{*http_client, MakeClientConfig(primary.GetBaseUrl(), {fallback.GetBaseUrl()})};

    EXPECT_EQ(client.FetchJson(std::nullopt, {}).timestamp, "primary");
    EXPECT_EQ(fallback.GetRequestsCount(), 0);
}

UTEST(DynamicConfigClient, SendsAdditionalRequestParams) {
    const ConfigsServerMock server{
        kStatusOk,
        "now",
        R"({"service":"service","updated_since":"previous","region":"eu","options":[true,3,{"tag":null}]})"
    };
    auto http_client = utest::CreateHttpClient();
    auto config = MakeClientConfig(server.GetBaseUrl(), {});
    config.additional_request_params = {
        {"region", formats::json::FromString(R"("eu")")},
        {"options", formats::json::FromString(R"([true,3,{"tag":null}])")},
    };
    Client client{*http_client, config};

    EXPECT_EQ(client.FetchJson("previous", {}).timestamp, "now");
}

UTEST_DEATH(ClientConfigDeathTest, AdditionalRequestParamsConflictWithStandardField) {
    auto http_client = utest::CreateHttpClient();
    auto config = MakeClientConfig("http://configs", {});
    config.additional_request_params.emplace("ids", formats::json::FromString("[]"));

    EXPECT_UINVARIANT_FAILURE_MSG(
        (Client{*http_client, config}),
        "additional-request-params contains standard request field 'ids'"
    );
}

UTEST(DynamicConfigClient, FallsBackOnServerError) {
    const ConfigsServerMock primary{kStatusInternalServerError, "primary"};
    const ConfigsServerMock first_fallback{kStatusInternalServerError, "first_fallback"};
    const ConfigsServerMock second_fallback{kStatusOk, "second_fallback"};
    auto http_client = utest::CreateHttpClient();
    Client client{
        *http_client,
        MakeClientConfig(primary.GetBaseUrl(), {first_fallback.GetBaseUrl(), second_fallback.GetBaseUrl()}),
    };

    EXPECT_EQ(client.FetchJson(std::nullopt, {}).timestamp, "second_fallback");
    EXPECT_EQ(primary.GetRequestsCount(), 1);
    EXPECT_EQ(first_fallback.GetRequestsCount(), 1);
}

UTEST(DynamicConfigClient, DoesNotFallBackOnClientError) {
    const ConfigsServerMock primary{kStatusBadRequest, "primary"};
    const ConfigsServerMock fallback{kStatusOk, "fallback"};
    auto http_client = utest::CreateHttpClient();
    Client client{*http_client, MakeClientConfig(primary.GetBaseUrl(), {fallback.GetBaseUrl()})};

    UEXPECT_THROW(client.FetchJson(std::nullopt, {}), clients::http::HttpClientException);
    EXPECT_EQ(fallback.GetRequestsCount(), 0);
}

UTEST_DEATH(ClientConfigDeathTest, EmptyFallbackUrl) {
    auto http_client = utest::CreateHttpClient();

    EXPECT_UINVARIANT_FAILURE_MSG(
        (Client{*http_client, MakeClientConfig("http://configs", {""})}),
        "Empty URL in 'fallback-config-urls'"
    );
}

UTEST_DEATH(ClientConfigDeathTest, FallbackUrlDuplicatesConfigUrl) {
    auto http_client = utest::CreateHttpClient();

    EXPECT_UINVARIANT_FAILURE_MSG(
        (Client{*http_client, MakeClientConfig("http://configs", {"http://configs"})}),
        "dynamic_config::ClientConfig fallback config URL 'http://configs' duplicates 'config-url'"
    );
}

}  // namespace dynamic_config

USERVER_NAMESPACE_END
