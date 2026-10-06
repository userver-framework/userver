#include <userver/utest/utest.hpp>

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <storages/mongo/util_mongotest.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/formats/bson/inline.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/multi_mongo.hpp>
#include <userver/storages/mongo/pool.hpp>
#include <userver/storages/mongo/pool_config.hpp>
#include <userver/storages/secdist/provider_component.hpp>
#include <userver/storages/secdist/secdist.hpp>
#include <userver/utils/resource_scopes.hpp>
#include <userver/utils/string_literal.hpp>

USERVER_NAMESPACE_BEGIN

namespace mongo = storages::mongo;

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
        void OnSecdistUpdate(const storages::secdist::SecdistConfig& secdist_config_update) {
            if (updates_counter == 1) {
                // prevents test flaps
                EXPECT_TRUE(file_updated.WaitForEventFor(utest::kMaxTestWaitTime));
            }

            if (updates_counter < 2) {
                secdist_config = secdist_config_update;
            }
            updates_counter++;
            if (updates_counter == 2) {
                updated_twice.Send();
            }
        };

        storages::secdist::SecdistConfig secdist_config;
        std::atomic<int> updates_counter{0};
        engine::SingleConsumerEvent file_updated{engine::SingleConsumerEvent::NoAutoReset{}};
        engine::SingleConsumerEvent updated_twice{engine::SingleConsumerEvent::NoAutoReset{}};
    };

    SecdistConfigStorage storage;
    auto dns_resolver = MakeDnsResolver();

    auto temp_file = fs::blocking::TempFile::Create();
    fs::blocking::RewriteFileContents(temp_file.GetPath(), kSecdistInitJson);

    storages::secdist::DefaultLoader provider{
        {temp_file.GetPath(),
         storages::secdist::SecdistFormat::kJson,
         false,
         std::nullopt,
         &engine::current_task::GetTaskProcessor(),
         {}}
    };
    storages::secdist::Secdist secdist{{&provider, std::chrono::milliseconds(100)}};
    auto subscriber =
        secdist.UpdateAndListen(&storage, "test/multimongo_update_secdist", &SecdistConfigStorage::OnSecdistUpdate);
    EXPECT_EQ(storage.updates_counter.load(), 1);

    const auto dynamic_config = MakeDynamicConfig();
    utils::WithResourceScopes<mongo::MultiMongo> multi_mongo(
        std::in_place,
        "userver_multimongo_test",
        secdist,
        MakeTestPoolConfig(),
        &dns_resolver,
        dynamic_config.GetSource()
    );

    UEXPECT_THROW(multi_mongo->AddPool("admin"), storages::mongo::InvalidConfigException);
    UEXPECT_THROW(multi_mongo->GetPool("admin"), storages::mongo::PoolNotFoundException);

    fs::blocking::RewriteFileContents(
        temp_file.GetPath(),
        fmt::format(kSecdistUpdateJsonFormat, GetTestsuiteMongoUri("admin"))
    );
    ASSERT_EQ(storage.updates_counter.load(), 1);
    storage.file_updated.Send();
    EXPECT_TRUE(storage.updated_twice.WaitForEventFor(utest::kMaxTestWaitTime));

    UEXPECT_NO_THROW(multi_mongo->AddPool("admin"));
    auto admin_pool = multi_mongo->GetPool("admin");

    static constexpr utils::StringLiteral kSysVerCollName = "system.version";

    EXPECT_TRUE(admin_pool->HasCollection(kSysVerCollName));
    UEXPECT_NO_THROW(admin_pool->GetCollection(std::string{kSysVerCollName}));
}

namespace {
class MultiMongoPool : public MongoPoolFixture {};
}  // namespace

UTEST_F(MultiMongoPool, DynamicSecdistUpdateAfterPoolCreation) {
    constexpr std::string_view k_secdist_json_format = R"({{"mongo_settings":{{"test":{{"uri":"{}"}}}}}})";
    constexpr auto k_update_period = std::chrono::milliseconds{100};
    constexpr auto k_poll_interval = std::chrono::milliseconds{10};
    const std::string k_other_database = kTestDatabaseNamePrefix + "multimongo_secdist_reload";
    const std::string k_collection = "secdist_reload";
    auto& old_pool = GetDefaultPool();
    auto new_pool = MakePool(k_other_database, {});
    old_pool.GetCollection(k_collection).InsertOne(formats::bson::MakeDoc("_id", 1));
    new_pool.GetCollection(k_collection).InsertOne(formats::bson::MakeDoc("_id", 2));

    auto dns_resolver = MakeDnsResolver();
    const auto dynamic_config = MakeDynamicConfig();
    auto temp_file = fs::blocking::TempFile::Create();
    fs::blocking::RewriteFileContents(
        temp_file.GetPath(),
        fmt::format(k_secdist_json_format, GetTestsuiteMongoUri(kTestDatabaseDefaultName))
    );
    storages::secdist::DefaultLoader provider{
        {temp_file.GetPath(),
         storages::secdist::SecdistFormat::kJson,
         false,
         std::nullopt,
         &engine::current_task::GetTaskProcessor(),
         {}}
    };
    storages::secdist::Secdist secdist{{&provider, k_update_period}};
    utils::WithResourceScopes<mongo::MultiMongo> multi_mongo(
        std::in_place,
        "userver_multimongo_reload_test",
        secdist,
        MakeTestPoolConfig(),
        &dns_resolver,
        dynamic_config.GetSource()
    );
    multi_mongo->AddPool("test");
    const auto pool = multi_mongo->GetPool("test");
    auto collection = pool->GetCollection(k_collection);
    ASSERT_EQ(1, collection.Count(formats::bson::MakeDoc("_id", 1)));
    ASSERT_EQ(0, collection.Count(formats::bson::MakeDoc("_id", 2)));

    fs::blocking::RewriteFileContents(
        temp_file.GetPath(),
        fmt::format(k_secdist_json_format, GetTestsuiteMongoUri(k_other_database))
    );
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
    while (collection.Count(formats::bson::MakeDoc("_id", 2)) == 0 && !deadline.IsReached()) {
        engine::SleepFor(k_poll_interval);
    }
    ASSERT_EQ(1, collection.Count(formats::bson::MakeDoc("_id", 2)));
    EXPECT_EQ(0, collection.Count(formats::bson::MakeDoc("_id", 1)));
    EXPECT_EQ(pool, multi_mongo->GetPool("test"));
    EXPECT_EQ(1, pool->GetCollection(k_collection).Count(formats::bson::MakeDoc("_id", 2)));

    collection.InsertOne(formats::bson::MakeDoc("_id", 3));
    EXPECT_EQ(1, new_pool.GetCollection(k_collection).Count(formats::bson::MakeDoc("_id", 3)));
    EXPECT_EQ(0, old_pool.GetCollection(k_collection).Count(formats::bson::MakeDoc("_id", 3)));
}

USERVER_NAMESPACE_END
