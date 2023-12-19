#include <userver/storages/postgres/transaction.hpp>

#include <storages/postgres/deadline.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/statement_timer.hpp>
#include <userver/storages/postgres/exceptions.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/uuid4.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

Transaction::Transaction(detail::ConnectionPtr&& conn,
                         const TransactionOptions& options,
                         OptionalCommandControl trx_cmd_ctl,
                         detail::SteadyClock::time_point trx_start_time)
    : conn_{std::move(conn)} {
  if (conn_) {
    conn_->Begin(options, trx_start_time, trx_cmd_ctl);
  }
}
Transaction::Transaction(Transaction&&) noexcept = default;
Transaction::~Transaction() {
  if (conn_ && conn_->IsInTransaction()) {
    LOG_INFO() << "Transaction handle is destroyed without an explicit "
                  "commit or rollback, rolling back automatically";
    try {
      conn_->Rollback();
    } catch (const std::exception& e) {
      LOG_LIMITED_ERROR()
          << "Exception when rolling back an abandoned transaction in "
             "destructor: "
          << e;
    }
  }
}

Transaction& Transaction::operator=(Transaction&& rhs) noexcept = default;

ResultSet Transaction::Execute(OptionalCommandControl statement_cmd_ctl,
                               const Query& query,
                               const ParameterStore& store) {
  return DoExecute(query, detail::QueryParameters{store.GetInternalData()},
                   statement_cmd_ctl);
}

Portal Transaction::MakePortal(OptionalCommandControl statement_cmd_ctl,
                               const Query& query,
                               const ParameterStore& store) {
  return MakePortal(PortalName{}, query,
                    detail::QueryParameters{store.GetInternalData()},
                    statement_cmd_ctl);
}

ResultSet Transaction::DoExecute(const Query& query,
                                 const detail::QueryParameters& params,
                                 OptionalCommandControl statement_cmd_ctl) {
  if (!conn_) {
    LOG_LIMITED_ERROR() << "Execute called after transaction finished"
                        << logging::LogExtra::Stacktrace();
    throw NotInTransaction("Transaction handle is not valid");
  }
  if (!statement_cmd_ctl) {
    statement_cmd_ctl = conn_->GetQueryCmdCtl(query.GetName());
  }
  auto source = conn_.GetConfigSource();
  if (source) CheckDeadlineIsExpired(source->GetSnapshot());

  detail::StatementTimer timer{query, conn_};
  auto res = conn_->Execute(query, params, std::move(statement_cmd_ctl));
  timer.Account();
  return res;
}

Portal Transaction::MakePortal(const PortalName& portal_name,
                               const Query& query,
                               const detail::QueryParameters& params,
                               OptionalCommandControl statement_cmd_ctl) {
  if (!conn_) {
    LOG_LIMITED_ERROR() << "Make portal called after transaction finished"
                        << logging::LogExtra::Stacktrace();
    throw NotInTransaction("Transaction handle is not valid");
  }
  if (portal_name.empty()) {
    // TODO: maybe forbid them altogether, as name collisions cause runtime
    // errors TAXICOMMON-4505
    return Portal{
        conn_.get(),
        PortalName{USERVER_NAMESPACE::utils::generators::GenerateUuid()}, query,
        params, std::move(statement_cmd_ctl)};
  }
  return Portal{conn_.get(), portal_name, query, params,
                std::move(statement_cmd_ctl)};
}

void Transaction::SetParameter(const std::string& param_name,
                               const std::string& value) {
  if (!conn_) {
    LOG_LIMITED_ERROR() << "Set parameter called after transaction finished"
                        << logging::LogExtra::Stacktrace();
    throw NotInTransaction("Transaction handle is not valid");
  }
  conn_->SetParameter(param_name, value,
                      detail::Connection::ParameterScope::kTransaction);
}

void Transaction::Commit() {
  if (conn_) {
    conn_->Commit();
    // in case of exception inside commit let it fly and don't release the
    // connection holder to allow for rolling back later
    conn_ = detail::ConnectionPtr{nullptr};
  } else {
    LOG_LIMITED_ERROR() << "Commit after transaction finished"
                        << logging::LogExtra::Stacktrace();
    throw NotInTransaction("Transaction handle is not valid");
  }
}

void Transaction::Rollback() {
  auto conn = std::move(conn_);
  if (conn) {
    conn->Rollback();
  } else {
    LOG_LIMITED_WARNING() << "Rollback after transaction finished"
                          << logging::LogExtra::Stacktrace();
  }
}

const UserTypes& Transaction::GetConnectionUserTypes() const {
  if (!conn_) {
    LOG_LIMITED_ERROR() << "Get user types called after transaction finished"
                        << logging::LogExtra::Stacktrace();
    throw NotInTransaction("Transaction handle is not valid");
  }
  return conn_->GetUserTypes();
}

OptionalCommandControl Transaction::GetConnTransactionCommandControlDebug()
    const {
  if (!conn_) return std::nullopt;
  return conn_->GetTransactionCommandControl();
}

TimeoutDuration Transaction::GetConnStatementTimeoutDebug() const {
  if (!conn_) return TimeoutDuration::zero();
  return conn_->GetStatementTimeout();
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
