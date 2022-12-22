#include <userver/utest/utest.hpp>

#include <chrono>
#include <string>
#include <vector>

#include <fmt/format.h>

#include <storages/mongo/util_mongotest.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/multi_mongo.hpp>
#include <userver/storages/mongo/pool.hpp>
#include <userver/storages/mongo/pool_config.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/storages/secdist/secdist.hpp>

USERVER_NAMESPACE_BEGIN

namespace mongo = storages::mongo;

namespace {
const mongo::PoolConfig kPoolConfig{};
}  // namespace

UTEST(MultiMongo, DynamicSecdistUpdate) {
  constexpr std::string_view kSecdistInitJson = R"~(
  {
      "mongo_settings": {
      }
  }
  )~";

  constexpr std::string_view kSecdistUpdateJsonFormat = R"~(
  {{
      "mongo_settings": {{
          "admin": {{
              "uri": "{}"
          }}
      }}
  }}
  )~";

  struct SecdistConfigStorage {
    void OnSecdistUpdate(
        const storages::secdist::SecdistConfig& secdist_config_update) {
      if (updates_counter == 1) {
        // prevents test flaps
        EXPECT_TRUE(file_updated.WaitForEventFor(utest::kMaxTestWaitTime));
      }

      if (updates_counter < 2) secdist_config = secdist_config_update;
      updates_counter++;
      if (updates_counter == 2) updated_twice.Send();
    };

    storages::secdist::SecdistConfig secdist_config;
    std::atomic<int> updates_counter{0};
    engine::SingleConsumerEvent file_updated{
        engine::SingleConsumerEvent::NoAutoReset{}};
    engine::SingleConsumerEvent updated_twice{
        engine::SingleConsumerEvent::NoAutoReset{}};
  };

  SecdistConfigStorage storage;
  auto dns_resolver = MakeDnsResolver();

  auto temp_file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(temp_file.GetPath(), kSecdistInitJson);

  storages::secdist::DefaultLoader provider{
      {temp_file.GetPath(), storages::secdist::SecdistFormat::kJson, false,
       std::nullopt, &engine::current_task::GetTaskProcessor()}};
  storages::secdist::Secdist secdist{
      {&provider, std::chrono::milliseconds(100)}};
  auto subscriber =
      secdist.UpdateAndListen(&storage, "test/multimongo_update_secdist",
                              &SecdistConfigStorage::OnSecdistUpdate);
  EXPECT_EQ(storage.updates_counter.load(), 1);

  const auto dynamic_config = MakeDynamicConfig();
  mongo::MultiMongo multi_mongo("userver_multimongo_test", secdist, kPoolConfig,
                                &dns_resolver, dynamic_config.GetSource());

  UEXPECT_THROW(multi_mongo.AddPool("admin"),
                storages::mongo::InvalidConfigException);
  UEXPECT_THROW(multi_mongo.GetPool("admin"),
                storages::mongo::PoolNotFoundException);

  fs::blocking::RewriteFileContents(
      temp_file.GetPath(),
      fmt::format(kSecdistUpdateJsonFormat, GetTestsuiteMongoUri("admin")));
  ASSERT_EQ(storage.updates_counter.load(), 1);
  storage.file_updated.Send();
  EXPECT_TRUE(storage.updated_twice.WaitForEventFor(utest::kMaxTestWaitTime));

  UEXPECT_NO_THROW(multi_mongo.AddPool("admin"));
  auto admin_pool = multi_mongo.GetPool("admin");

  static const std::string kSysVerCollName = "system.version";

  EXPECT_TRUE(admin_pool->HasCollection(kSysVerCollName));
  UEXPECT_NO_THROW(admin_pool->GetCollection(kSysVerCollName));
}

USERVER_NAMESPACE_END
