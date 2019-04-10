#include <storages/postgres/transaction.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/exceptions.hpp>

#include <logging/log.hpp>

namespace storages {
namespace postgres {

Transaction::Transaction(detail::ConnectionPtr&& conn,
                         const TransactionOptions& options,
                         OptionalCommandControl trx_cmd_ctl,
                         detail::SteadyClock::time_point&& trx_start_time)
    : conn_{std::move(conn)} {
  if (conn_) {
    // NOLINTNEXTLINE(hicpp-move-const-arg)
    conn_->Begin(options, std::move(trx_start_time), trx_cmd_ctl);
  }
}
Transaction::Transaction(Transaction&&) noexcept = default;
Transaction::~Transaction() {
  if (conn_ && conn_->IsInTransaction()) {
    LOG_ERROR() << "Transaction handle is destroyed without implicitly "
                   "committing or rolling back"
                << logging::LogExtra::Stacktrace();
    try {
      conn_->Rollback();
    } catch (const std::exception& e) {
      LOG_ERROR() << "Exception when rolling back an abandoned transaction in "
                     "destructor: "
                  << e;
    }
  }
}

Transaction& Transaction::operator=(Transaction&& rhs) noexcept = default;

ResultSet Transaction::DoExecute(const std::string& statement,
                                 const detail::QueryParameters& params,
                                 OptionalCommandControl statement_cmd_ctl) {
  if (!conn_) {
    LOG_ERROR() << "Execute called after transaction finished"
                << logging::LogExtra::Stacktrace();
    throw NotInTransaction("Transaction handle is not valid");
  }
  return conn_->Execute(statement, params, std::move(statement_cmd_ctl));
}

void Transaction::SetParameter(const std::string& param_name,
                               const std::string& value) {
  if (!conn_) {
    LOG_ERROR() << "Set parameter called after transaction finished"
                << logging::LogExtra::Stacktrace();
    throw NotInTransaction("Transaction handle is not valid");
  }
  conn_->SetParameter(param_name, value,
                      detail::Connection::ParameterScope::kTransaction);
}

void Transaction::Commit() {
  auto conn = std::move(conn_);
  if (conn) {
    conn->Commit();
  } else {
    LOG_ERROR() << "Commit after transaction finished"
                << logging::LogExtra::Stacktrace();
    throw NotInTransaction("Transaction handle is not valid");
  }
}

void Transaction::Rollback() {
  auto conn = std::move(conn_);
  if (conn) {
    conn->Rollback();
  } else {
    LOG_WARNING() << "Rollback after transaction finished"
                  << logging::LogExtra::Stacktrace();
  }
}

const UserTypes& Transaction::GetConnectionUserTypes() const {
  if (!conn_) {
    LOG_ERROR() << "Get user types called after transaction finished"
                << logging::LogExtra::Stacktrace();
    throw NotInTransaction("Transaction handle is not valid");
  }
  return conn_->GetUserTypes();
}

}  // namespace postgres
}  // namespace storages
