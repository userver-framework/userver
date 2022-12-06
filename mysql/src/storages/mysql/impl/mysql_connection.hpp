#pragma once

#include <string>
#include <vector>

#include <mysql/mysql.h>

#include <storages/mysql/impl/mysql_result.hpp>
#include <storages/mysql/impl/mysql_socket.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class MySQLConnection final {
 public:
  MySQLConnection();
  ~MySQLConnection();

  MySQLResult Execute(const std::string& query, engine::Deadline deadline);

 private:
  MySQLSocket InitSocket();

  void MySQLExecuteQuery(const std::string& query, engine::Deadline deadline);
  MySQLResult MySQLFetchResult(engine::Deadline deadline);

  MYSQL mysql_;
  MYSQL* connect_ret_{nullptr};

  MySQLSocket socket_;
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
