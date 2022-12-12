#pragma once

#include <atomic>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <userver/utils/str_icase.hpp>

#include <storages/mysql/impl/mariadb_include.hpp>

#include <storages/mysql/impl/mysql_result.hpp>
#include <storages/mysql/impl/mysql_socket.hpp>
#include <storages/mysql/impl/mysql_statement.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {
namespace settings {
class HostSettings;
}

namespace io {
class ExtractorBase;
class ParamsBinderBase;
}  // namespace io
}  // namespace storages::mysql

namespace storages::mysql::impl {

class MySQLConnection final {
 public:
  MySQLConnection(const settings::HostSettings& host_settings,
                  engine::Deadline deadline);
  ~MySQLConnection();

  MySQLResult ExecutePlain(const std::string& query, engine::Deadline deadline);

  MySQLStatementFetcher ExecuteStatement(const std::string& statement,
                                         io::ParamsBinderBase& params,
                                         engine::Deadline deadline);

  void ExecuteInsert(const std::string& insert_statement,
                     io::ParamsBinderBase& params, engine::Deadline deadline);

  void Ping(engine::Deadline deadline);

  MySQLSocket& GetSocket();
  MYSQL& GetNativeHandler();

  bool IsBroken() const;

  const char* GetNativeError();
  std::string GetNativeError(std::string_view prefix);

  std::string EscapeString(std::string_view source);

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
  MySQLSocket InitSocket();

  MySQLStatement& PrepareStatement(const std::string& statement,
                                   engine::Deadline deadline);

  const settings::HostSettings& host_settings_;
  const std::string host_ip_;

  std::atomic<bool> broken_{false};

  MYSQL mysql_;
  MYSQL* connect_ret_{nullptr};

  MySQLSocket socket_;

  std::unordered_map<std::string, MySQLStatement, utils::StrIcaseHash,
                     utils::StrIcaseEqual>
      statements_cache_;
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
