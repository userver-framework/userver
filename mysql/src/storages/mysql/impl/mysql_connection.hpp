#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <mysql/mysql.h>

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

  void ExecuteStatement(const std::string& statement,
                        io::ParamsBinderBase& params,
                        io::ExtractorBase& extractor,
                        engine::Deadline deadline);

  MySQLSocket& GetSocket();
  MYSQL& GetNativeHandler();

  const char* GetNativeError();
  std::string GetNativeError(std::string_view prefix);

 private:
  MySQLSocket InitSocket();

  MySQLStatement& PrepareStatement(const std::string& statement,
                                   engine::Deadline deadline);

  MYSQL mysql_;
  MYSQL* connect_ret_{nullptr};

  MySQLSocket socket_;

  std::unordered_map<std::string, MySQLStatement> statements_cache_;
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
