#include <userver/utest/utest.hpp>

#include "utils_test.hpp"

#include <userver/storages/clickhouse/cluster.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(Cluster, NoAvailablePools) {
  ClusterWrapper cluster{
      false,
      {{"unresolved1", 11111}, {"unresolved2", 12222}, {"unresolved3", 12333}}};

  EXPECT_THROW(cluster->Execute("Invalid_query"),
               storages::clickhouse::Cluster::NoAvailablePoolError);
}

USERVER_NAMESPACE_END
