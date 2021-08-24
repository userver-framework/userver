#include <userver/utest/utest.hpp>

#include <chrono>
#include <string>
#include <vector>

#include <userver/storages/mongo/multi_mongo.hpp>

#include <userver/engine/sleep.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/pool.hpp>
#include <userver/storages/mongo/pool_config.hpp>
#include <userver/storages/secdist/secdist.hpp>

using namespace storages::mongo;

namespace {
const PoolConfig kPoolConfig("userver_multimongo_test",
                             PoolConfig::DriverImpl::kMongoCDriver);
}  // namespace

UTEST(MultiMongo, DynamicSecdistUpdate) {
  const std::string kSecdistInitJson = R"~(
  {
      "mongo_settings": {
      }
  }
  )~";

  const std::string kSecdistUpdateJson = R"~(
  {
      "mongo_settings": {
          "admin": {
              "uri": "mongodb://localhost:27217/admin"
          }
      }
  }
  )~";

  struct SecdistConfigStorage {
    void OnSecdistUpdate(
        const storages::secdist::SecdistConfig& secdist_config_update) {
      if (updates_counter == 1) {
        // prevents test flaps
        while (!file_updated.load()) {
          engine::SleepFor(std::chrono::milliseconds(1));
        }
      }

      if (updates_counter < 2) secdist_config = secdist_config_update;
      updates_counter++;
    };

    storages::secdist::SecdistConfig secdist_config;
    std::atomic<int> updates_counter{0};
    std::atomic<bool> file_updated{false};
  };

  SecdistConfigStorage storage;

  auto temp_file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(temp_file.GetPath(), kSecdistInitJson);

  storages::secdist::Secdist secdist(
      {temp_file.GetPath(), false, std::nullopt, std::chrono::milliseconds(100),
       &engine::current_task::GetTaskProcessor()});
  auto subscriber =
      secdist.UpdateAndListen(&storage, "test/multimongo_update_secdist",
                              &SecdistConfigStorage::OnSecdistUpdate);
  EXPECT_EQ(storage.updates_counter.load(), 1);

  MultiMongo multi_mongo("userver_multimongo_test", secdist, kPoolConfig);

  EXPECT_THROW(multi_mongo.AddPool("admin"),
               storages::mongo::InvalidConfigException);
  EXPECT_THROW(multi_mongo.GetPool("admin"),
               storages::mongo::PoolNotFoundException);

  fs::blocking::RewriteFileContents(temp_file.GetPath(), kSecdistUpdateJson);
  ASSERT_EQ(storage.updates_counter.load(), 1);
  storage.file_updated = true;
  while (storage.updates_counter.load() < 2) {
    engine::SleepFor(std::chrono::milliseconds(1));
  }

  EXPECT_NO_THROW(multi_mongo.AddPool("admin"));
  auto admin_pool = multi_mongo.GetPool("admin");

  static const std::string kSysVerCollName = "system.version";

  EXPECT_TRUE(admin_pool->HasCollection(kSysVerCollName));
  EXPECT_NO_THROW(admin_pool->GetCollection(kSysVerCollName));
}
