#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <userver/urabbitmq/client_settings.hpp>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/deadline.hpp>

#include <urabbitmq/statistics/connection_statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

class ConnectionPool;
class ConnectionPtr;

class ClientImpl final {
 public:
  ClientImpl(clients::dns::Resolver& resolver, const ClientSettings& settings);

  ConnectionPtr GetConnection(engine::Deadline deadline);

  void WriteStatistics(utils::statistics::Writer& writer) const;

 private:
  const ClientSettings settings_;

  struct PoolHolder final {
    statistics::ConnectionStatistics stats;
    std::shared_ptr<ConnectionPool> pool;
  };
  std::vector<PoolHolder> pools_;
  std::atomic<size_t> pool_idx_{0};
};

}  // namespace urabbitmq

USERVER_NAMESPACE_END
