#pragma once

#include <userver/engine/deadline.hpp>

#include <storages/mysql/impl/mariadb_include.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class Socket;

// This class encapsulates some machinery with foo_start/foo_cont
class NativeInterface final {
 public:
  NativeInterface(Socket& socket, engine::Deadline deadline);

  int StatementPrepare(MYSQL_STMT* stmt, const char* stmt_str,
                       std::size_t length) &&;
  int StatementExecute(MYSQL_STMT* stmt) &&;
  int StatementStoreResult(MYSQL_STMT* stmt) &&;
  int StatementFetch(MYSQL_STMT* stmt) &&;
  my_bool StatementFreeResult(MYSQL_STMT* stmt) &&;
  my_bool StatementClose(MYSQL_STMT* stmt) &&;

  int QueryExecute(MYSQL* mysql, const char* stmt_str, std::size_t length) &&;
  MYSQL_RES* QueryStoreResult(MYSQL* mysql) &&;
  MYSQL_ROW QueryResultFetchRow(MYSQL_RES* result) &&;
  void QueryFreeResult(MYSQL_RES* result) &&;

  int Ping(MYSQL* mysql) &&;
  my_bool Commit(MYSQL* mysql) &&;
  my_bool Rollback(MYSQL* mysql) &&;
  void Close(MYSQL* mysql) &&;

 private:
  Socket& socket_;
  engine::Deadline deadline_;
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
