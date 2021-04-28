#include <storages/postgres/detail/non_transaction.hpp>

#include <storages/postgres/detail/connection.hpp>

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
  return DoExecute(statement, store.GetInternalData(), statement_cmd_ctl);
}

ResultSet NonTransaction::DoExecute(const Query& query,
                                    const detail::QueryParameters& params,
                                    OptionalCommandControl statement_cmd_ctl) {
  return conn_->Execute(query, params, statement_cmd_ctl);
}

const UserTypes& NonTransaction::GetConnectionUserTypes() const {
  return conn_->GetUserTypes();
}

}  // namespace storages::postgres::detail
