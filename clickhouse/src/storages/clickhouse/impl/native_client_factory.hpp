#pragma once

#include <memory>
#include <string>

#include <storages/clickhouse/impl/wrap_clickhouse_cpp.hpp>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/deadline.hpp>

#include <storages/clickhouse/impl/connection_mode.hpp>

namespace clickhouse {
struct ClientOptions;
class Query;
class Block;
class Client;
}  // namespace clickhouse

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

struct EndpointSettings;
struct AuthSettings;

namespace impl {

class NativeClientWrapper final {
 public:
  NativeClientWrapper(clients::dns::Resolver*,
                      const clickhouse_cpp::ClientOptions&, ConnectionMode);
  ~NativeClientWrapper();

  void Execute(const clickhouse_cpp::Query& query, engine::Deadline deadline);
  void Insert(const std::string& table_name, const clickhouse_cpp::Block& block,
              engine::Deadline deadline);
  void Ping(engine::Deadline deadline);

 private:
  void SetDeadline(engine::Deadline deadline);

  engine::Deadline::Duration connect_timeout_;
  engine::Deadline operations_deadline_;

  std::unique_ptr<clickhouse_cpp::Client> native_client_;
};

class NativeClientFactory final {
 public:
  static NativeClientWrapper Create(clients::dns::Resolver*,
                                    const EndpointSettings&,
                                    const AuthSettings&, ConnectionMode);
};

}  // namespace impl
}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
