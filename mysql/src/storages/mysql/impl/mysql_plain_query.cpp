#include <storages/mysql/impl/mysql_plain_query.hpp>

#include <memory>

#include <storages/mysql/impl/mariadb_include.hpp>

#include <storages/mysql/impl/mysql_connection.hpp>
#include <storages/mysql/impl/mysql_native_interface.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

namespace {

class NativeResultDeleter final {
 public:
  NativeResultDeleter(MySQLConnection& connection, engine::Deadline deadline)
      : connection_{connection}, deadline_{deadline} {}

  void operator()(MYSQL_RES* native_result) {
    try {
      MySQLNativeInterface{connection_.GetSocket(), deadline_}.QueryFreeResult(
          native_result);
    } catch (const std::exception& ex) {
      LOG_WARNING() << "Failed to correctly dispose a query result: "
                    << ex.what();
    }
  }

 private:
  MySQLConnection& connection_;
  engine::Deadline deadline_;
};

}  // namespace

MySQLPlainQuery::MySQLPlainQuery(MySQLConnection& connection,
                                 const std::string& query)
    : connection_{&connection}, query_{query} {}

MySQLPlainQuery::~MySQLPlainQuery() = default;

MySQLPlainQuery::MySQLPlainQuery(MySQLPlainQuery&& other) noexcept = default;

void MySQLPlainQuery::Execute(BrokenGuard& guard, engine::Deadline deadline) {
  int err =
      MySQLNativeInterface{connection_->GetSocket(), deadline}.QueryExecute(
          &connection_->GetNativeHandler(), query_.data(), query_.length());

  if (err != 0) {
    guard.ThrowCommandException(
        mysql_errno(&connection_->GetNativeHandler()),
        connection_->GetNativeError("Failed to execute a query: "));
  }
}

MySQLResult MySQLPlainQuery::FetchResult(BrokenGuard&,
                                         engine::Deadline deadline) {
  std::unique_ptr<MYSQL_RES, NativeResultDeleter> native_result{
      MySQLNativeInterface{connection_->GetSocket(), deadline}.QueryStoreResult(
          &connection_->GetNativeHandler()),
      NativeResultDeleter{*connection_, deadline}};
  if (!native_result) {
    // Well, query doesn't return any result set
    return {};
  }

  MySQLResult result{};
  while (true) {
    MYSQL_ROW row = MySQLNativeInterface{connection_->GetSocket(), deadline}
                        .QueryResultFetchRow(native_result.get());
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
