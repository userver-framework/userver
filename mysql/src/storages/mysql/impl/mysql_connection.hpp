#pragma once

#include <atomic>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <userver/clients/dns/resolver_fwd.hpp>

#include <storages/mysql/impl/mariadb_include.hpp>

#include <storages/mysql/impl/metadata/mysql_server_info.hpp>
#include <storages/mysql/impl/mysql_result.hpp>
#include <storages/mysql/impl/mysql_socket.hpp>
#include <storages/mysql/impl/mysql_statement.hpp>
#include <storages/mysql/impl/mysql_statements_cache.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace settings {
struct EndpointInfo;
struct AuthSettings;
}  // namespace settings

namespace impl {

namespace io {
class ParamsBinderBase;
}  // namespace io

class MySQLConnection final {
 public:
  MySQLConnection(clients::dns::Resolver& resolver,
                  const settings::EndpointInfo& endpoint_info,
                  const settings::AuthSettings& auth_settings,
                  engine::Deadline deadline);
  ~MySQLConnection();

  MySQLResult ExecutePlain(const std::string& query, engine::Deadline deadline);

  MySQLStatementFetcher ExecuteStatement(const std::string& statement,
                                         io::ParamsBinderBase& params,
                                         engine::Deadline deadline,
                                         std::optional<std::size_t> batch_size);

  void ExecuteInsert(const std::string& insert_statement,
                     io::ParamsBinderBase& params, engine::Deadline deadline);

  void Ping(engine::Deadline deadline);

  void Commit(engine::Deadline deadline);
  void Rollback(engine::Deadline deadline);

  MySQLSocket& GetSocket();
  MYSQL& GetNativeHandler();

  bool IsBroken() const;

  const char* GetNativeError();
  std::string GetNativeError(std::string_view prefix);

  std::string EscapeString(std::string_view source);

  const metadata::MySQLServerInfo& GetServerInfo() const;

  class BrokenGuard final {
   public:
    explicit BrokenGuard(MySQLConnection& connection);
    ~BrokenGuard();

   private:
    int exceptions_on_enter_;
    std::atomic<bool>& broken_;
  };
  BrokenGuard GetBrokenGuard();

 private:
  void InitSocket(clients::dns::Resolver& resolver,
                  const settings::EndpointInfo& endpoint_info,
                  const settings::AuthSettings& auth_settings,
                  engine::Deadline deadline);
  bool DoInitSocket(const std::string& ip, std::uint32_t port,
                    const settings::AuthSettings& auth_settings,
                    engine::Deadline deadline);
  void Close(engine::Deadline deadline) noexcept;

  MySQLStatement& PrepareStatement(const std::string& statement,
                                   engine::Deadline deadline,
                                   std::optional<std::size_t> batch_size);

  std::atomic<bool> broken_{false};

  MYSQL mysql_{};
  MYSQL* connect_ret_{nullptr};

  MySQLSocket socket_;
  metadata::MySQLServerInfo server_info_{};

  MySQLStatementsCache statements_cache_;
};

}  // namespace impl
}  // namespace storages::mysql

USERVER_NAMESPACE_END
