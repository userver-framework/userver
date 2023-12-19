#include <storages/mysql/impl/plain_query.hpp>

#include <memory>

#include <storages/mysql/impl/mariadb_include.hpp>

#include <storages/mysql/impl/connection.hpp>
#include <storages/mysql/impl/native_interface.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

namespace {

class NativeResultDeleter {
 public:
  NativeResultDeleter(Connection& connection, engine::Deadline deadline)
      : connection_{connection}, deadline_{deadline} {}

  void operator()(MYSQL_RES* native_result) const noexcept {
    try {
      NativeInterface{connection_.GetSocket(), deadline_}.QueryFreeResult(
          native_result);
    } catch (const std::exception& ex) {
      LOG_WARNING() << "Failed to correctly dispose a query result: "
                    << ex.what();
    }
  }

 private:
  Connection& connection_;
  engine::Deadline deadline_;
};

}  // namespace

PlainQuery::PlainQuery(Connection& connection, const std::string& query)
    : connection_{&connection}, query_{query} {}

PlainQuery::~PlainQuery() = default;

PlainQuery::PlainQuery(PlainQuery&& other) noexcept = default;

void PlainQuery::Execute(engine::Deadline deadline) {
  const int err =
      NativeInterface{connection_->GetSocket(), deadline}.QueryExecute(
          &connection_->GetNativeHandler(), query_.data(), query_.length());

  if (err != 0) {
    throw MySQLCommandException{
        mysql_errno(&connection_->GetNativeHandler()),
        connection_->GetNativeError("Failed to execute a query: ")};
  }
}

QueryResult PlainQuery::FetchResult(engine::Deadline deadline) {
  const std::unique_ptr<MYSQL_RES, NativeResultDeleter> native_result{
      NativeInterface{connection_->GetSocket(), deadline}.QueryStoreResult(
          &connection_->GetNativeHandler()),
      NativeResultDeleter{*connection_, deadline}};
  if (!native_result) {
    // Well, query doesn't return any result set
    return {};
  }

  QueryResult result{};
  while (true) {
    MYSQL_ROW row =
        NativeInterface{connection_->GetSocket(), deadline}.QueryResultFetchRow(
            native_result.get());
    if (!row) {
      break;
    }

    result.AppendRow(
        QueryResultRow{row, mysql_field_count(&connection_->GetNativeHandler()),
                       mysql_fetch_lengths(native_result.get())});
  }

  return result;
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
