#include <userver/ugrpc/client/client_factory.hpp>

#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/formats/yaml/value_builder.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <../include/userver/ugrpc/client/impl/completion_queue_pool.hpp>
#include <userver/ugrpc/client/channels.hpp>
#include <userver/ugrpc/client/client_factory.hpp>
#include <userver/ugrpc/client/impl/client_data.hpp>
#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/tests/service_fixtures.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class GrpcClientWithServiceConfig : public ugrpc::tests::ServiceFixtureBase {
protected:
    static constexpr std::string_view kServiceConfig = R"json(
  {
    "methodConfig": [
      {
        "name": [
          {
            "service": "s1",
            "method": "m1"
          },
          {
          "service": "s2",
          "method": "m2"
          }
        ],
        "timeout" : "1.0004s"
      }
    ]
  }
  )json";

    GrpcClientWithServiceConfig() {
        // Sanity-check. It is harder to understand errors inside grpc-core,
        // so check for json-ness here
        UEXPECT_NO_THROW(formats::json::FromString(kServiceConfig));

        ugrpc::client::ClientFactorySettings settings;
        settings.channel_args.SetServiceConfigJSON(grpc::string{kServiceConfig});

        StartServer(std::move(settings));
    }

    ~GrpcClientWithServiceConfig() override { StopServer(); }
};

}  // namespace

UTEST_F(GrpcClientWithServiceConfig, DefaultServiceConfig) {
    auto client = MakeClient<sample::ugrpc::UnitTestServiceClient>();

    auto& data = ugrpc::client::impl::GetClientData(client);

    // This is a very important moment. THe default service_config is applied not
    // upon creation, but when the name resolution process returns no service
    // config from DNS. We need to trigger the name resolution, and the easiest
    // way is to attempt to connect. TryWaitForConnected does exactly that.
    // It doesn't matter whether we connect or not - we only need the name
    // resolution process to execute.
    std::ignore = ugrpc::client::TryWaitForConnected(
        client, engine::Deadline::FromDuration(std::chrono::milliseconds{50}), engine::current_task::GetTaskProcessor()
    );

    // test that service_config was passed to gRPC Core
    auto& token = data.GetChannelToken();
    for (std::size_t i = 0; i < token.GetChannelCount(); ++i) {
        ASSERT_EQ(kServiceConfig, ugrpc::impl::ToString(token.GetChannel(i)->GetServiceConfigJSON()));
    }
}

USERVER_NAMESPACE_END
