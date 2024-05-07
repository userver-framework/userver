#include <userver/storages/postgres/detail/non_transaction.hpp>
#include <userver/testsuite/testpoint.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/detail/statement_stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

NonTransaction::NonTransaction(ConnectionPtr&& conn,
                               detail::SteadyClock::time_point start_time)
    : conn_{std::move(conn)} {
  conn_->Start(start_time);
}

NonTransaction::NonTransaction(NonTransaction&&) noexcept = default;
NonTransaction::~NonTransaction() { conn_->Finish(); }

NonTransaction& NonTransaction::operator=(NonTransaction&&) noexcept = default;

ResultSet NonTransaction::Execute(OptionalCommandControl statement_cmd_ctl,
                                  const std::string& statement,
                                  const ParameterStore& store) {
  return DoExecute(statement, detail::QueryParameters{store.GetInternalData()},
                   statement_cmd_ctl);
}

ResultSet NonTransaction::DoExecute(const Query& query,
                                    const detail::QueryParameters& params,
                                    OptionalCommandControl statement_cmd_ctl) {
  if (query.GetName().has_value()) {
    TESTPOINT_CALLBACK(
        fmt::format("pg_ntrx_execute::{}", query.GetName().value()),
        formats::json::Value(), [](const formats::json::Value& data) {
          if (data["inject_failure"].As<bool>()) {
            LOG_WARNING() << "Failing statement "
                             "due to Testpoint response";
            throw std::runtime_error{"Statement failed"};
          }
        });
  }

  StatementStats stats{query, conn_};
  try {
    auto res = conn_->Execute(query, params, statement_cmd_ctl);
    stats.AccountStatementExecution();
    return res;
  } catch (const std::exception& e) {
    stats.AccountStatementError();
    throw;
  }
}

const UserTypes& NonTransaction::GetConnectionUserTypes() const {
  return conn_->GetUserTypes();
}

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
