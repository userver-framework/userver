#include <userver/dynamic_config/client/client.hpp>

#include <userver/utest/assert_macros.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

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

}  // namespace dynamic_config

USERVER_NAMESPACE_END
