#include <storages/postgres/transaction.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/exceptions.hpp>

#include <logging/log.hpp>

namespace storages {
namespace postgres {

namespace {

const detail::QueryParameters kEmptyParams;

}  // namespace

Transaction::Transaction(detail::ConnectionPtr&& conn,
                         const TransactionOptions& options)
    : conn_{std::move(conn)} {
  if (conn_) {
    conn_->Begin(options);
  }
}
Transaction::Transaction(Transaction&&) noexcept = default;
Transaction::~Transaction() {
  if (conn_ && conn_->IsInTransaction()) {
    LOG_ERROR() << "Transaction handle is destroyed without implicitly "
                   "commiting or rolling back";
    conn_->Rollback();
  }
}

Transaction& Transaction::operator=(Transaction&& rhs) = default;

ResultSet Transaction::Execute(const std::string& statement) {
  if (!conn_) {
    // TODO Log stacktrace
    throw NotInTransaction("Transaction handle is not valid");
  }
  return conn_->Execute(statement, kEmptyParams);
}

ResultSet Transaction::DoExecute(const std::string& statement,
                                 const detail::QueryParameters& params) {
  if (!conn_) {
    // TODO Log stacktrace
    throw NotInTransaction("Transaction handle is not valid");
  }
  return conn_->Execute(statement, params);
}

void Transaction::Commit() {
  auto conn = std::move(conn_);
  if (conn) {
    conn->Commit();
  } else {
    // TODO Log stacktrace
    throw NotInTransaction("Transaction handle is not valid");
  }
}

void Transaction::Rollback() {
  auto conn = std::move(conn_);
  if (conn) {
    conn->Rollback();
  } else {
    // TODO Log stacktrace
    throw NotInTransaction("Transaction handle is not valid");
  }
}

}  // namespace postgres
}  // namespace storages
