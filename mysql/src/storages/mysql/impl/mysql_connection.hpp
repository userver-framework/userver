#pragma once

#include <atomic>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <storages/mysql/impl/mariadb_include.hpp>

#include <storages/mysql/impl/mysql_result.hpp>
#include <storages/mysql/impl/mysql_socket.hpp>
#include <storages/mysql/impl/mysql_statement.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::io {
class ExtractorBase;
class ParamsBinderBase;
}  // namespace storages::mysql::io

namespace storages::mysql::impl {

class MySQLConnection final {
 public:
  MySQLConnection();
  ~MySQLConnection();

  MySQLResult ExecutePlain(const std::string& query, engine::Deadline deadline);

  MySQLStatementFetcher ExecuteStatement(const std::string& statement,
                                         io::ParamsBinderBase& params,
                                         engine::Deadline deadline);

  void ExecuteInsert(const std::string& insert_statement,
                     io::ParamsBinderBase& params, std::size_t rows_count,
                     engine::Deadline deadline);

  void Ping(engine::Deadline deadline);

  MySQLSocket& GetSocket();
  MYSQL& GetNativeHandler();

  bool IsBroken() const;

  const char* GetNativeError();
  std::string GetNativeError(std::string_view prefix);

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

  std::atomic<bool> broken_{false};

  MYSQL mysql_;
  MYSQL* connect_ret_{nullptr};

  MySQLSocket socket_;

  std::unordered_map<std::string, MySQLStatement> statements_cache_;
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
