#pragma once

#include <userver/clients/dns/resolver.hpp>
#include <userver/storages/clickhouse/cluster.hpp>

USERVER_NAMESPACE_BEGIN

class ClusterWrapper final {
 public:
  ClusterWrapper();

  storages::clickhouse::Cluster* operator->();
  storages::clickhouse::Cluster& operator*();

 private:
  clients::dns::Resolver resolver_;
  storages::clickhouse::Cluster cluster_;
};

USERVER_NAMESPACE_END
