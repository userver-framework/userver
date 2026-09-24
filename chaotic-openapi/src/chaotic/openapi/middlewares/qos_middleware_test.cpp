#include <userver/chaotic/openapi/middlewares/qos_middleware.hpp>

#include <string>

#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/utest/log_capture_fixture.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

const dynamic_config::Key<chaotic::openapi::client::CommandControlDict> kQosConfig{
    "CHAOTIC_OPENAPI_TEST_CLIENT_QOS",
    dynamic_config::DefaultAsJsonString{R"({"__default__":{"timeout-ms":1,"attempts":1}})"},
};

const auto kQosWithInvalidMethod = formats::json::FromString(R"({
  "__default__": {"timeout-ms": 1, "attempts": 1},
  "/ok@get": {"timeout-ms": 2, "attempts": 1},
  "/bad@head": {"timeout-ms": 3, "attempts": 1}
})");

class QosMiddlewareLog : public utest::LogCaptureFixture<> {};

}  // namespace

UTEST_F(QosMiddlewareLog, InvalidMethodLogContainsConfigName) {
    const dynamic_config::StorageMock storage{{kQosConfig, kQosWithInvalidMethod}};
    const auto snapshot = storage.GetSnapshot();
    chaotic::openapi::impl::WarnOnInvalidQosConfig(snapshot, kQosConfig);

    const auto logs = GetLogCapture().Filter(kQosConfig.GetName());
    ASSERT_EQ(logs.size(), 1);
    EXPECT_NE(logs.front().GetText().find("in dynamic config CHAOTIC_OPENAPI_TEST_CLIENT_QOS"), std::string::npos);
    EXPECT_NE(logs.front().GetText().find("invalid method 'head'"), std::string::npos);
}

UTEST_F(QosMiddlewareLog, AllowedMethodDoesNotLog) {
    const auto qos = formats::json::FromString(R"({
      "__default__": {"timeout-ms": 1, "attempts": 1},
      "/ok@get": {"timeout-ms": 2, "attempts": 1}
    })");
    const dynamic_config::StorageMock storage{{kQosConfig, qos}};
    const auto snapshot = storage.GetSnapshot();
    chaotic::openapi::impl::WarnOnInvalidQosConfig(snapshot, kQosConfig);

    EXPECT_TRUE(GetLogCapture().Filter("invalid method").empty());
}

USERVER_NAMESPACE_END
