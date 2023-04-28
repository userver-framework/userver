#pragma once

#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <userver/utest/utest.hpp>

#include <userver/clients/dns/resolver.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

#include <userver/storages/mongo/pool.hpp>
#include <userver/storages/mongo/pool_config.hpp>

USERVER_NAMESPACE_BEGIN

extern const std::string kTestDatabaseNamePrefix;
extern const std::string kTestDatabaseDefaultName;

std::string GetTestsuiteMongoUri(const std::string& database);

clients::dns::Resolver MakeDnsResolver();

dynamic_config::StorageMock MakeDynamicConfig();

class MongoPoolFixture : public ::testing::Test {
 protected:
  MongoPoolFixture();
  ~MongoPoolFixture() override;

  storages::mongo::Pool GetDefaultPool();

  storages::mongo::Pool MakePool(
      std::optional<std::string> db_name,
      std::optional<storages::mongo::PoolConfig> config,
      std::optional<clients::dns::Resolver*> dns_resolver = {});

  void SetDynamicConfig(const std::vector<dynamic_config::KeyValue>& config);

 private:
  utils::impl::UserverExperimentsScope experiments_;
  clients::dns::Resolver default_resolver_;
  dynamic_config::StorageMock dynamic_config_storage_;
  std::unordered_set<std::string> used_db_names_;
  storages::mongo::Pool default_pool_;
};

USERVER_NAMESPACE_END
