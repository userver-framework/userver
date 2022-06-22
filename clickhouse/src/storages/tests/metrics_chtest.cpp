#include <userver/utest/utest.hpp>

#include <storages/clickhouse/impl/connection_ptr.hpp>

#include "utils_test.hpp"

namespace {

struct DummyData {
  std::vector<std::string> data;
};

}  // namespace

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::io {

template <>
struct CppToClickhouse<DummyData> {
  using mapped_type = std::tuple<columns::StringColumn>;
};

}  // namespace storages::clickhouse::io

UTEST(Metrics, Basic) {
  ClusterWrapper cluster{};

  const DummyData data{{"str"}};

  // successful query
  cluster->Execute("CREATE TEMPORARY TABLE IF NOT EXISTS tmp(value String)");
  // successful insert
  cluster->Insert("tmp", {"value"}, data);

  EXPECT_EQ(cluster->GetStatistics()["localhost"]["connections"]["active"]
                .As<uint64_t>(),
            1);

  // unsuccessful query
  EXPECT_ANY_THROW(cluster->Execute("invalid_query_format"));
  // unsuccessful insert
  EXPECT_ANY_THROW(cluster->Insert("nonexistent", {"value"}, data));

  const auto cluster_stats = cluster->GetStatistics();
  const auto stats = cluster_stats["localhost"];

  const auto connection_stats = stats["connections"];
  const auto queries_stats = stats["queries"];
  const auto insert_stats = stats["inserts"];

  EXPECT_EQ(connection_stats["closed"].As<uint64_t>(), 2);

  EXPECT_EQ(queries_stats["total"].As<uint64_t>(), 2);
  EXPECT_EQ(queries_stats["error"].As<uint64_t>(), 1);

  EXPECT_EQ(insert_stats["total"].As<uint64_t>(), 2);
  EXPECT_EQ(insert_stats["error"].As<uint64_t>(), 1);
}

UTEST(Metrics, ActiveConnections) {
  PoolWrapper pool{};

  std::vector<storages::clickhouse::impl::ConnectionPtr> connections;
  for (size_t i = 0; i < 3; ++i) {
    connections.emplace_back(pool->Acquire());
  }

  EXPECT_EQ(pool->GetStatistics().connections.busy, 3);
  EXPECT_EQ(pool->GetStatistics().connections.active, 3);
  connections.clear();
  EXPECT_EQ(pool->GetStatistics().connections.busy, 0);
  EXPECT_EQ(pool->GetStatistics().connections.active, 3);
}

USERVER_NAMESPACE_END
