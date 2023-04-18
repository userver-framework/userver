#include <storages/mysql/impl/native_interface.hpp>

#include <userver/logging/log.hpp>

#include <storages/mysql/impl/socket.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

NativeInterface::NativeInterface(Socket& socket, engine::Deadline deadline)
    : socket_{socket}, deadline_{deadline} {}

int NativeInterface::StatementPrepare(MYSQL_STMT* stmt, const char* stmt_str,
                                      std::size_t length) && {
  int err = 0;
  socket_.RunToCompletion(
      [&err, stmt, stmt_str, length] {
        return mysql_stmt_prepare_start(&err, stmt, stmt_str, length);
      },
      [&err, stmt](int mysql_events) {
        return mysql_stmt_prepare_cont(&err, stmt, mysql_events);
      },
      deadline_);

  return err;
}

int NativeInterface::StatementExecute(MYSQL_STMT* stmt) && {
  int err = 0;
  socket_.RunToCompletion(
      [&err, stmt] { return mysql_stmt_execute_start(&err, stmt); },
      [&err, stmt](int mysql_events) {
        return mysql_stmt_execute_cont(&err, stmt, mysql_events);
      },
      deadline_);

  return err;
}

int NativeInterface::StatementStoreResult(MYSQL_STMT* stmt) && {
  int err = 0;
  socket_.RunToCompletion(
      [&err, stmt] { return mysql_stmt_store_result_start(&err, stmt); },
      [&err, stmt](int mysql_events) {
        return mysql_stmt_store_result_cont(&err, stmt, mysql_events);
      },
      deadline_);

  return err;
}

int NativeInterface::StatementFetch(MYSQL_STMT* stmt) && {
  int err = 0;
  socket_.RunToCompletion(
      [&err, stmt] { return mysql_stmt_fetch_start(&err, stmt); },
      [&err, stmt](int mysql_events) {
        return mysql_stmt_fetch_cont(&err, stmt, mysql_events);
      },
      deadline_);

  return err;
}

my_bool NativeInterface::StatementFreeResult(MYSQL_STMT* stmt) && {
  my_bool err{};
  socket_.RunToCompletion(
      [&err, stmt] { return mysql_stmt_free_result_start(&err, stmt); },
      [&err, stmt](int mysql_events) {
        return mysql_stmt_free_result_cont(&err, stmt, mysql_events);
      },
      deadline_);

  return err;
}

my_bool NativeInterface::StatementClose(MYSQL_STMT* stmt) && {
  my_bool err{};
  socket_.RunToCompletion(
      [&err, stmt] { return mysql_stmt_close_start(&err, stmt); },
      [&err, stmt](int mysql_events) {
        return mysql_stmt_close_cont(&err, stmt, mysql_events);
      },
      deadline_);

  return err;
}

int NativeInterface::QueryExecute(MYSQL* mysql, const char* stmt_str,
                                  std::size_t length) && {
  int err = 0;
  socket_.RunToCompletion(
      [&err, mysql, stmt_str, length] {
        return mysql_real_query_start(&err, mysql, stmt_str, length);
      },
      [&err, mysql](int mysql_events) {
        return mysql_real_query_cont(&err, mysql, mysql_events);
      },
      deadline_);

  return err;
}

MYSQL_RES* NativeInterface::QueryStoreResult(MYSQL* mysql) && {
  MYSQL_RES* result{nullptr};

  socket_.RunToCompletion(
      [&result, mysql] { return mysql_store_result_start(&result, mysql); },
      [&result, mysql](int mysql_events) {
        return mysql_store_result_cont(&result, mysql, mysql_events);
      },
      deadline_);

  return result;
}

MYSQL_ROW NativeInterface::QueryResultFetchRow(MYSQL_RES* result) && {
  MYSQL_ROW row{nullptr};
  socket_.RunToCompletion(
      [&row, result] { return mysql_fetch_row_start(&row, result); },
      [&row, result](int mysql_events) {
        return mysql_fetch_row_cont(&row, result, mysql_events);
      },
      deadline_);

  return row;
}

void NativeInterface::QueryFreeResult(MYSQL_RES* result) && {
  socket_.RunToCompletion([result] { return mysql_free_result_start(result); },
                          [result](int mysql_events) {
                            return mysql_free_result_cont(result, mysql_events);
                          },
                          deadline_);
}

int NativeInterface::Ping(MYSQL* mysql) && {
  int err = 0;
  socket_.RunToCompletion(
      [&err, mysql] { return mysql_ping_start(&err, mysql); },
      [&err, mysql](int mysql_events) {
        return mysql_ping_cont(&err, mysql, mysql_events);
      },
      deadline_);

  return err;
}

my_bool NativeInterface::Commit(MYSQL* mysql) && {
  my_bool err = 0;
  socket_.RunToCompletion(
      [&err, mysql] { return mysql_commit_start(&err, mysql); },
      [&err, mysql](int mysql_events) {
        return mysql_commit_cont(&err, mysql, mysql_events);
      },
      deadline_);

  return err;
}

my_bool NativeInterface::Rollback(MYSQL* mysql) && {
  my_bool err = 0;
  socket_.RunToCompletion(
      [&err, mysql] { return mysql_rollback_start(&err, mysql); },
      [&err, mysql](int mysql_events) {
        return mysql_rollback_cont(&err, mysql, mysql_events);
      },
      deadline_);

  return err;
}

void NativeInterface::Close(MYSQL* mysql) && {
  try {
    socket_.RunToCompletion([mysql] { return mysql_close_start(mysql); },
                            [mysql](int mysql_event) {
                              return mysql_close_cont(mysql, mysql_event);
                            },
                            deadline_);
  } catch (const std::exception& ex) {
    // Us being here means we timed out on sending COM_QUIT.
    // We must drive mysql_close_cont to completion, otherwise connection gets
    // leaked, but mysql_close_cont won't go any further until socket is
    // unblocked, so we `shutdown` the socket.
    // https://jira.mariadb.org/browse/CONC-621
    mariadb_cancel(mysql);

    mysql_close_cont(mysql, 0);

    LOG_WARNING() << "Failed to correctly close a connection, forcibly "
                     "shutting it down. Reason: "
                  << ex.what();
  }
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
