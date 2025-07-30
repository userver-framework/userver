#include <userver/storages/clickhouse/parameter_store.hpp>
#include <userver/storages/clickhouse/query.hpp>
#include <userver/utest/utest.hpp>

#include "utils_test.hpp"

USERVER_NAMESPACE_BEGIN

UTEST(ClickhouseParameterStore, ParamsEqualPlaceholders) {
    ClusterWrapper cluster;
    cluster->Execute(storages::Query{"CREATE TABLE IF NOT EXISTS users (id UInt64, name String) ENGINE = Memory"});
    /// [basic usage]
    storages::Query q{"SELECT {} FROM {} WHERE id = {}"};
    storages::clickhouse::ParameterStore params;
    params.PushBack("name");
    params.PushBack("users");
    params.PushBack(42);
    EXPECT_NO_THROW(cluster->Execute(q, params));
    /// [basic usage]
}

UTEST(ClickhouseParameterStore, LessParamsThanPlaceholders) {
    ClusterWrapper cluster;
    cluster->Execute(storages::Query{"CREATE TABLE IF NOT EXISTS users (id UInt64, name String) ENGINE = Memory"});
    storages::Query q{"SELECT {} FROM {} WHERE id = {}"};
    storages::clickhouse::ParameterStore params1;
    params1.PushBack("name");
    params1.PushBack("users");
    EXPECT_THROW(cluster->Execute(q, params1), std::runtime_error);

    storages::clickhouse::ParameterStore params2;
    params2.PushBack("name");
    EXPECT_THROW(cluster->Execute(q, params2), std::runtime_error);

    storages::clickhouse::ParameterStore params3;
    EXPECT_THROW(cluster->Execute(q, params3), std::runtime_error);
}

UTEST(ClickhouseParameterStore, MoreParamsThanPlaceholders) {
    ClusterWrapper cluster;
    cluster->Execute(storages::Query{"CREATE TABLE IF NOT EXISTS users (id UInt64, name String) ENGINE = Memory"});
    storages::Query q{"SELECT {} FROM {} WHERE id = {}"};
    storages::clickhouse::ParameterStore params;
    params.PushBack("name");
    params.PushBack("users");
    params.PushBack(42);
    params.PushBack("extra");
    EXPECT_NO_THROW(cluster->Execute(q, params));
}

UTEST(ClickhouseParameterStore, NoPlaceholdersNoParams) {
    ClusterWrapper cluster;
    storages::Query q{"SELECT 1"};
    storages::clickhouse::ParameterStore params;
    EXPECT_NO_THROW(cluster->Execute(q, params));
}

UTEST(ClickhouseParameterStore, NoPlaceholdersWithParams) {
    ClusterWrapper cluster;
    storages::Query q{"SELECT 1"};
    storages::clickhouse::ParameterStore params;
    params.PushBack(1);
    EXPECT_NO_THROW(cluster->Execute(q, params));
}

UTEST(ClickhouseParameterStore, OnlyBraces) {
    ClusterWrapper cluster;
    storages::Query q{"SELECT {}"};
    storages::clickhouse::ParameterStore params1;
    params1.PushBack(1);
    EXPECT_NO_THROW(cluster->Execute(q, params1));

    storages::clickhouse::ParameterStore params2;
    EXPECT_THROW(cluster->Execute(q, params2), std::runtime_error);

    storages::clickhouse::ParameterStore params3;
    params3.PushBack(1);
    params3.PushBack(2);
    EXPECT_NO_THROW(cluster->Execute(q, params3));
}

UTEST(ClickhouseParameterStore, MultipleBracesCases) {
    ClusterWrapper cluster;
    storages::Query q{"SELECT {}, {}, {}"};
    storages::clickhouse::ParameterStore params1;
    params1.PushBack(1);
    params1.PushBack(2);
    params1.PushBack(3);
    EXPECT_NO_THROW(cluster->Execute(q, params1));

    storages::clickhouse::ParameterStore params2;
    params2.PushBack(1);
    params2.PushBack(2);
    EXPECT_THROW(cluster->Execute(q, params2), std::runtime_error);

    storages::clickhouse::ParameterStore params3;
    params3.PushBack(1);
    params3.PushBack(2);
    params3.PushBack(3);
    params3.PushBack(4);
    EXPECT_NO_THROW(cluster->Execute(q, params3));
}

UTEST(ClickhouseParameterStore, IndexedPlaceholdersExactParams) {
    ClusterWrapper cluster;
    cluster->Execute(storages::Query{"CREATE TABLE IF NOT EXISTS users (id UInt64, name String) ENGINE = Memory"});
    storages::Query q{"SELECT {1} FROM {0} WHERE id = {2}"};
    storages::clickhouse::ParameterStore params;
    params.PushBack("users");
    params.PushBack("name");
    params.PushBack(42);
    EXPECT_NO_THROW(cluster->Execute(q, params));
}

UTEST(ClickhouseParameterStore, IndexedPlaceholdersLessParams) {
    ClusterWrapper cluster;
    storages::Query q{"SELECT {0}, {1}, {2} FROM users"};
    storages::clickhouse::ParameterStore params;
    params.PushBack("id");
    params.PushBack("name");
    EXPECT_THROW(cluster->Execute(q, params), std::runtime_error);
}

UTEST(ClickhouseParameterStore, IndexedPlaceholdersMoreParams) {
    ClusterWrapper cluster;
    storages::Query q{"SELECT {2}, {1}, {0} FROM users"};
    storages::clickhouse::ParameterStore params;
    params.PushBack("name");
    params.PushBack("id");
    params.PushBack("age");
    params.PushBack("extra");
    EXPECT_NO_THROW(cluster->Execute(q, params));
}

UTEST(ClickhouseParameterStore, IndexedPlaceholdersRepeatedIndexes) {
    ClusterWrapper cluster;
    storages::Query q{"SELECT {0}, {0}, {1} FROM users"};
    storages::clickhouse::ParameterStore params;
    params.PushBack("id");
    params.PushBack("name");
    EXPECT_NO_THROW(cluster->Execute(q, params));
}

UTEST(ClickhouseParameterStore, IndexedPlaceholdersNonSequential) {
    ClusterWrapper cluster;
    storages::Query q{"SELECT {2}, {0}, {5} FROM users"};
    storages::clickhouse::ParameterStore params;
    params.PushBack("first");
    params.PushBack("second");
    params.PushBack("third");
    params.PushBack("fourth");
    params.PushBack("fifth");
    params.PushBack("sixth");
    EXPECT_NO_THROW(cluster->Execute(q, params));
}

UTEST(ClickhouseParameterStore, IndexedPlaceholdersTooFewParamsForMaxIndex) {
    ClusterWrapper cluster;
    storages::Query q{"SELECT {2}, {0}, {5} FROM users"};
    storages::clickhouse::ParameterStore params;
    params.PushBack("first");
    params.PushBack("second");
    params.PushBack("third");
    EXPECT_THROW(cluster->Execute(q, params), std::runtime_error);
}

UTEST(ClickhouseParameterStore, IndexedPlaceholdersZeroParams) {
    ClusterWrapper cluster;
    storages::Query q{"SELECT {0} FROM users"};
    storages::clickhouse::ParameterStore params;
    EXPECT_THROW(cluster->Execute(q, params), std::runtime_error);
}

USERVER_NAMESPACE_END
