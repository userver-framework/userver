#include <storages/mysql/impl/mysql_plain_query.hpp>

#include <memory>

#include <mysql/mysql.h>

#include <storages/mysql/impl/mysql_connection.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

namespace {

class NativeResultDeleter final {
 public:
  NativeResultDeleter(MySQLConnection& connection) : connection_{connection} {}

  void operator()(MYSQL_RES* native_result) {
    try {
      connection_.GetSocket().RunToCompletion(
          [native_result] { return mysql_free_result_start(native_result); },
          [native_result](int mysql_events) {
            return mysql_free_result_cont(native_result, mysql_events);
          },
          {} /* TODO : deadline */
      );
    } catch (const std::exception& ex) {
      LOG_WARNING() << "Failed to correctly dispose a query result: "
                    << ex.what();
    }
  }

 private:
  MySQLConnection& connection_;
};

}  // namespace

MySQLPlainQuery::MySQLPlainQuery(MySQLConnection& connection,
                                 const std::string& query)
    : connection_{&connection}, query_{query} {}

MySQLPlainQuery::~MySQLPlainQuery() = default;

MySQLPlainQuery::MySQLPlainQuery(MySQLPlainQuery&& other) noexcept = default;

void MySQLPlainQuery::Execute(engine::Deadline deadline) {
  int err = 0;
  MYSQL* native_handler = &connection_->GetNativeHandler();
  connection_->GetSocket().RunToCompletion(
      [this, &err, native_handler] {
        return mysql_real_query_start(&err, native_handler, query_.data(),
                                      query_.length());
      },
      [&err, native_handler](int mysql_events) {
        return mysql_real_query_cont(&err, native_handler, mysql_events);
      },
      deadline);

  if (err != 0) {
    throw std::runtime_error{
        connection_->GetNativeError("Failed to execute a query: ")};
  }
}

MySQLResult MySQLPlainQuery::FetchResult(engine::Deadline deadline) {
  std::unique_ptr<MYSQL_RES, NativeResultDeleter> native_result{
      mysql_use_result(&connection_->GetNativeHandler()),
      NativeResultDeleter{*connection_}};
  if (!native_result) {
    throw std::runtime_error{"Failed to fetch a query result"};
  }

  MySQLResult result{};
  MYSQL_ROW row{nullptr};
  while (true) {
    connection_->GetSocket().RunToCompletion(
        [&row, &native_result] {
          return mysql_fetch_row_start(&row, native_result.get());
        },
        [&row, &native_result](int mysql_events) {
          return mysql_fetch_row_cont(&row, native_result.get(), mysql_events);
        },
        deadline);
    if (!row) {
      break;
    }

    result.AppendRow(
        MySQLRow{row, mysql_field_count(&connection_->GetNativeHandler()),
                 mysql_fetch_lengths(native_result.get())});
  }

  return result;
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
