#include <userver/ugrpc/client/client_factory.hpp>

#include <userver/engine/task/task.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/formats/yaml/value_builder.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <userver/ugrpc/client/channels.hpp>
#include <userver/ugrpc/client/impl/client_data.hpp>
#include <userver/ugrpc/client/queue_holder.hpp>

#include <tests/unit_test_client.usrv.pb.hpp>
#include "userver/dynamic_config/storage_mock.hpp"

USERVER_NAMESPACE_BEGIN

UTEST(GrpcClient, ChannelsCount) {
  constexpr int kChannelsCount = 4;
  formats::yaml::ValueBuilder builder(formats::common::Type::kObject);
  builder["channel-count"] = kChannelsCount;

  const auto yaml_data = builder.ExtractValue();
  yaml_config::YamlConfig yaml_config(yaml_data, formats::yaml::Value());

  auto config = yaml_config.As<ugrpc::client::ClientFactoryConfig>();
  ugrpc::client::QueueHolder client_queue;
  utils::statistics::Storage statistics_storage;
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

  ASSERT_EQ(kChannelsCount, data.GetChannelToken().GetChannelCount());
}

USERVER_NAMESPACE_END
