#include <userver/utest/utest.hpp>

#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/storages/redis/impl/thread_pools.hpp>
#include <userver/utils/enumerate.hpp>

#include <storages/redis/client_impl.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/util_redistest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

storages::redis::ClientPtr GetClientWithResolver(
    clients::dns::Resolver* resolver) {
  auto thread_pools = std::make_shared<redis::ThreadPools>(
      redis::kDefaultSentinelThreadPoolSize,
      redis::kDefaultRedisThreadPoolSize);
  auto sentinel = redis::Sentinel::CreateSentinel(
      std::move(thread_pools), GetTestsuiteRedisSettings(), "none", "pub",
      redis::KeyShardFactory{""}, redis::kDefaultCommandControl, {}, resolver);
  sentinel->WaitConnectedDebug();
  return std::make_shared<storages::redis::ClientImpl>(std::move(sentinel));
}

std::shared_ptr<redis::Sentinel> CreateSentinelCustomResolve(
    const std::shared_ptr<redis::ThreadPools>& thread_pools,
    const secdist::RedisSettings& settings, std::string shard_group_name,
    const std::string& client_name,
    redis::Sentinel::ReadyChangeCallback ready_callback,
    redis::KeyShardFactory key_shard_factory,
    const redis::CommandControl& command_control,
    const testsuite::RedisControl& testsuite_redis_control,
    std::vector<redis::ConnectionInfo::HostVector> resolved_hosts) {
  const auto& password = settings.password;

  const std::vector<std::string>& shards = settings.shards;

  const bool is_same_size = settings.sentinels.size() == resolved_hosts.size();
  EXPECT_TRUE(is_same_size);
  if (!is_same_size) return {};

  std::vector<redis::ConnectionInfo> conns;
  conns.reserve(settings.sentinels.size());
  auto key_shard = key_shard_factory(shards.size());
  for (const auto& [i, sentinel] : utils::enumerate(settings.sentinels)) {
    conns.emplace_back(sentinel.host, sentinel.port,
                       (key_shard ? redis::Password("") : password), false,
                       settings.secure_connection, resolved_hosts[i]);
  }

  std::shared_ptr<redis::Sentinel> client;
  if (!shards.empty() && !conns.empty()) {
    client = std::make_shared<redis::Sentinel>(
        thread_pools, shards, conns, std::move(shard_group_name), client_name,
        password, settings.secure_connection, std::move(ready_callback),
        std::move(key_shard), command_control, testsuite_redis_control,
        redis::ConnectionMode::kCommands);
    client->Start();
  }

  return client;
}

storages::redis::ClientPtr GetClientCustomResolve(
    std::vector<redis::ConnectionInfo::HostVector> resolved_hosts,
    redis::Sentinel::ReadyChangeCallback ready_callback) {
  auto thread_pools = std::make_shared<redis::ThreadPools>(
      redis::kDefaultSentinelThreadPoolSize,
      redis::kDefaultRedisThreadPoolSize);
  auto sentinel = CreateSentinelCustomResolve(
      std::move(thread_pools), GetTestsuiteRedisSettings(), "none", "pub",
      std::move(ready_callback), redis::KeyShardFactory{""},
      redis::kDefaultCommandControl, {}, resolved_hosts);
  sentinel->WaitConnectedOnce({redis::WaitConnectedMode::kMasterAndSlave, false,
                               std::chrono::milliseconds(2000)});
  return std::make_shared<storages::redis::ClientImpl>(std::move(sentinel));
}

}  // namespace

UTEST(RedisClient, DnsResolver) {
  auto resolver = clients::dns::Resolver{
      engine::current_task::GetTaskProcessor(),
      {},
  };

  auto client = GetClientWithResolver(&resolver);
  EXPECT_NO_THROW(client->Set("key0", "foo", {}).Get());
  EXPECT_EQ(client->Get("key0", {}).Get(), "foo");
}

UTEST(RedisClient, FailedResolve) {
  redis::ConnectionInfo::HostVector failed_resolved_host = {};
  const auto size = GetTestsuiteRedisSettings().sentinels.size();
  std::vector<redis::ConnectionInfo::HostVector> resolved_hosts(
      size, failed_resolved_host);

  bool result = false;
  auto ready_callback = [&](size_t, const std::string&, bool ready) {
    result = ready;
  };

  auto client =
      GetClientCustomResolve(resolved_hosts, std::move(ready_callback));
  EXPECT_NO_THROW(client->Set("key0", "foo", {}).Get());
  EXPECT_EQ(client->Get("key0", {}).Get(), "foo");
  EXPECT_TRUE(result);
}

UTEST(RedisClient, IncorrectResolve) {
  redis::ConnectionInfo::HostVector incorrect_resolved_host = {"incorrect1",
                                                               "incorrect2"};
  const auto size = GetTestsuiteRedisSettings().sentinels.size();
  std::vector<redis::ConnectionInfo::HostVector> resolved_hosts(
      size, incorrect_resolved_host);

  bool result = false;
  auto ready_callback = [&](size_t, const std::string&, bool ready) {
    result = ready;
  };

  auto client =
      GetClientCustomResolve(resolved_hosts, std::move(ready_callback));
  EXPECT_FALSE(result);
}

UTEST(RedisClient, PartiallyIncorrectResolve) {
  redis::ConnectionInfo::HostVector partially_incorrect_resolved_host = {
      "incorrect"};
  const auto& settings = GetTestsuiteRedisSettings();
  const auto size = settings.sentinels.size();
  std::vector<redis::ConnectionInfo::HostVector> resolved_hosts(
      size, partially_incorrect_resolved_host);
  for (std::size_t i = 0; i < size; i++)
    resolved_hosts[i].emplace_back(settings.sentinels[i].host);

  bool result = false;
  auto ready_callback = [&](size_t, const std::string&, bool ready) {
    result = ready;
  };

  auto client =
      GetClientCustomResolve(resolved_hosts, std::move(ready_callback));
  EXPECT_NO_THROW(client->Set("key0", "foo", {}).Get());
  EXPECT_EQ(client->Get("key0", {}).Get(), "foo");
  EXPECT_TRUE(result);
}

USERVER_NAMESPACE_END
