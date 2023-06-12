#include <userver/ugrpc/client/client_factory.hpp>

#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/formats/yaml/value_builder.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/client/channels.hpp>
#include <userver/ugrpc/client/impl/client_data.hpp>
#include <userver/ugrpc/client/queue_holder.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(GrpcClient, DefaultServiceConfig) {
  utils::statistics::Storage statistics_storage;

  const std::string service_config{R"json(
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
  )json"};

  // Sanity-check. It is harder to understand errors inside grpc-core,
  // so check for json-ness here
  ASSERT_NO_THROW(formats::json::FromString(service_config));

  formats::yaml::ValueBuilder builder(formats::common::Type::kObject);
  builder["default-service-config"] = service_config;

  const auto yaml_data = builder.ExtractValue();
  yaml_config::YamlConfig yaml_config(yaml_data, formats::yaml::Value());

  auto config = yaml_config.As<ugrpc::client::ClientFactoryConfig>();
  ugrpc::client::QueueHolder client_queue;
  dynamic_config::StorageMock config_storage;

  testsuite::GrpcControl ts({}, false);
  ugrpc::client::MiddlewareFactories mws;
  ugrpc::client::ClientFactory client_factory(
      std::move(config), engine::current_task::GetTaskProcessor(), mws,
      client_queue.GetQueue(), statistics_storage, ts,
      config_storage.GetSource());
  const std::string endpoint{"[::]:50051"};
  auto client = client_factory.MakeClient<sample::ugrpc::UnitTestServiceClient>(
      "test", endpoint);

  auto& data = ugrpc::client::impl::GetClientData(client);

  // This is a very important moment. THe default service_config is applied not
  // upon creation, but when the name resolution process returns no service
  // config from DNS. We need to trigger the name resolution, and the easiest
  // way is to attempt to connect. TryWaitForConnected does exactly that.
  // It doesn't matter whether we connect or not - we only need the name
  // resolution process to execute.
  std::ignore = ugrpc::client::TryWaitForConnected(
      client, engine::Deadline::FromDuration(std::chrono::milliseconds{50}),
      engine::current_task::GetTaskProcessor());

  // test that service_config was passed to gRPC Core
  auto& token = data.GetChannelToken();
  for (std::size_t i = 0; i < token.GetChannelCount(); ++i) {
    ASSERT_EQ(service_config, ugrpc::impl::ToString(
                                  token.GetChannel(i)->GetServiceConfigJSON()));
  }
}

USERVER_NAMESPACE_END
