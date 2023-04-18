#pragma once

#include <atomic>
#include <string>
#include <string_view>
#include <vector>

#include <userver/clients/dns/resolver_fwd.hpp>

#include <storages/mysql/impl/mariadb_include.hpp>

#include <storages/mysql/impl/broken_guard.hpp>
#include <storages/mysql/impl/metadata/server_info.hpp>
#include <storages/mysql/impl/query_result.hpp>
#include <storages/mysql/impl/socket.hpp>
#include <storages/mysql/impl/statement.hpp>
#include <storages/mysql/impl/statements_cache.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace settings {
struct EndpointInfo;
struct AuthSettings;
struct ConnectionSettings;
}  // namespace settings

namespace impl {

namespace io {
class ParamsBinderBase;
}  // namespace io

class Connection final {
 public:
  Connection(clients::dns::Resolver& resolver,
             const settings::EndpointInfo& endpoint_info,
             const settings::AuthSettings& auth_settings,
             const settings::ConnectionSettings& connection_settings,
             engine::Deadline deadline);
  ~Connection();

  QueryResult ExecuteQuery(const std::string& query, engine::Deadline deadline);

  StatementFetcher ExecuteStatement(const std::string& statement,
                                    io::ParamsBinderBase& params,
                                    engine::Deadline deadline,
                                    std::optional<std::size_t> batch_size);

  void Ping(engine::Deadline deadline);

  void Commit(engine::Deadline deadline);
  void Rollback(engine::Deadline deadline);

  Socket& GetSocket();
  MYSQL& GetNativeHandler();

  bool IsBroken() const;

  std::string GetNativeError(std::string_view prefix);

  const metadata::ServerInfo& GetServerInfo() const;

  BrokenGuard GetBrokenGuard();

  // There are places (destructors, basically) where we want to run some
  // function even if connection is already broken, because that function frees
  // resources no matter what. Can't use BrokenGuard for that, 'cause it will
  // throw on construction, but still need a way no notify a connection that it
  // broke.
  void NotifyBroken();

 private:
  void InitSocket(clients::dns::Resolver& resolver,
                  const settings::EndpointInfo& endpoint_info,
                  const settings::AuthSettings& auth_settings,
                  const settings::ConnectionSettings& connection_settings,
                  engine::Deadline deadline);
  bool DoInitSocket(const std::string& ip, std::uint32_t port,
                    const settings::AuthSettings& auth_settings,
                    const settings::ConnectionSettings& connection_settings,
                    engine::Deadline deadline);
  void Close(engine::Deadline deadline) noexcept;

  Statement& PrepareStatement(const std::string& statement,
                              engine::Deadline deadline,
                              std::optional<std::size_t> batch_size);

  std::atomic<bool> broken_{false};

  Socket socket_;
  MYSQL mysql_{};

  metadata::ServerInfo server_info_{};

  StatementsCache statements_cache_;
};

}  // namespace impl
}  // namespace storages::mysql

USERVER_NAMESPACE_END
