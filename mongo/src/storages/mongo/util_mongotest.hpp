#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <userver/utest/utest.hpp>

#include <userver/clients/dns/resolver.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/utils/impl/source_location.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

#include <userver/storages/mongo/pool.hpp>
#include <userver/storages/mongo/pool_config.hpp>
#include <userver/storages/mongo/write_result.hpp>

#include <storages/mongo/features.hpp>

USERVER_NAMESPACE_BEGIN

std::string GetTestDatabaseNamePrefix();
std::string GetTestDatabaseDefaultName();

inline constexpr std::uint32_t kDuplicateKeyErrorCode = 11000;

std::string GetTestsuiteMongoUri(const std::string& database);

clients::dns::Resolver MakeDnsResolver();

dynamic_config::StorageMock MakeDynamicConfig();

storages::mongo::PoolConfig MakePoolConfigForTest(bool experimental);

std::vector<bool> GetMongoPoolImplementations();
std::string GetMongoPoolImplementationName(const ::testing::TestParamInfo<bool>& info);

struct ExpectedWriteCounts final {
    std::size_t inserted{0};
    std::size_t matched{0};
    std::size_t modified{0};
    std::size_t upserted{0};
    std::size_t deleted{0};
};

void ExpectWriteCounts(
    const storages::mongo::WriteResult& result,
    const ExpectedWriteCounts& expected,
    const utils::impl::SourceLocation& source_location = utils::impl::SourceLocation::Current()
);

void ExpectNoWriteErrors(
    const storages::mongo::WriteResult& result,
    const utils::impl::SourceLocation& source_location = utils::impl::SourceLocation::Current()
);

void ExpectSameWriteCounts(
    const storages::mongo::WriteResult& native,
    const storages::mongo::WriteResult& bulk_write,
    const utils::impl::SourceLocation& source_location = utils::impl::SourceLocation::Current()
);

void ExpectSingleUpsertedId(
    const storages::mongo::WriteResult& result,
    int expected_id,
    const utils::impl::SourceLocation& source_location = utils::impl::SourceLocation::Current()
);

void ExpectSingleDuplicateKeyError(
    const storages::mongo::WriteResult& result,
    const utils::impl::SourceLocation& source_location = utils::impl::SourceLocation::Current()
);

class MongoPoolFixture : public ::testing::TestWithParam<bool> {
protected:
    MongoPoolFixture();
    ~MongoPoolFixture() override;

    storages::mongo::Pool& GetDefaultPool();

    storages::mongo::PoolConfig MakeTestPoolConfig() const;

    storages::mongo::Pool MakePool(
        std::optional<std::string> db_name,
        std::optional<storages::mongo::PoolConfig> config,
        std::optional<clients::dns::Resolver*> dns_resolver = {}
    );

    storages::mongo::Pool MakeUnacknowledgedPool(const std::string& db_name = GetTestDatabaseDefaultName());

    void SetDynamicConfig(const std::vector<dynamic_config::KeyValue>& config);

private:
    utils::impl::UserverExperimentsScope experiments_;
    clients::dns::Resolver default_resolver_;
    dynamic_config::StorageMock dynamic_config_storage_;
    std::unordered_set<std::string> used_db_names_;
    storages::mongo::Pool default_pool_;
};

USERVER_NAMESPACE_END
