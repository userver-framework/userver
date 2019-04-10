#include <storages/postgres/detail/non_transaction.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/exceptions.hpp>

#include <logging/log.hpp>

namespace storages::postgres::detail {

NonTransaction::NonTransaction(ConnectionPtr&& conn, engine::Deadline deadline,
                               detail::SteadyClock::time_point start_time)
    : conn_{std::move(conn)}, deadline_{deadline} {
  // NOLINTNEXTLINE(hicpp-move-const-arg)
  conn_->Start(std::move(start_time));
}

NonTransaction::NonTransaction(NonTransaction&&) noexcept = default;
NonTransaction::~NonTransaction() { conn_->Finish(); }

NonTransaction& NonTransaction::operator=(NonTransaction&&) noexcept = default;

ResultSet NonTransaction::DoExecute(const std::string& statement,
                                    const detail::QueryParameters& params,
                                    OptionalCommandControl statement_cmd_ctl) {
  return conn_->Execute(statement, params, deadline_,
                        std::move(statement_cmd_ctl));
}

const UserTypes& NonTransaction::GetConnectionUserTypes() const {
  return conn_->GetUserTypes();
}

}  // namespace storages::postgres::detail
