#pragma once

#include <string>

#include <userver/storages/postgres/options.hpp>
#include <userver/storages/postgres/parameter_store.hpp>
#include <userver/storages/postgres/postgres_fwd.hpp>
#include <userver/storages/postgres/query.hpp>
#include <userver/storages/postgres/result_set.hpp>

#include <userver/storages/postgres/detail/connection_ptr.hpp>
#include <userver/storages/postgres/detail/query_parameters.hpp>
#include <userver/storages/postgres/detail/time_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

class NonTransaction {
 public:
  explicit NonTransaction(
      ConnectionPtr&& conn,
      SteadyClock::time_point start_time = detail::SteadyClock::now());

  NonTransaction(NonTransaction&&) noexcept;
  NonTransaction& operator=(NonTransaction&&) noexcept;

  NonTransaction(const NonTransaction&) = delete;
  NonTransaction& operator=(const NonTransaction&) = delete;

  ~NonTransaction();

  /// @name Query execution
  /// @{
  /// Execute statement with arbitrary parameters.
  ///
  /// Suspends coroutine for execution.
  template <typename... Args>
  ResultSet Execute(const Query& query, const Args&... args) {
    return Execute(OptionalCommandControl{}, query, args...);
  }

  /// Execute statement with arbitrary parameters and per-statement command
  /// control.
  ///
  /// Suspends coroutine for execution.
  template <typename... Args>
  ResultSet Execute(OptionalCommandControl statement_cmd_ctl,
                    const Query& query, const Args&... args) {
    detail::StaticQueryParameters<sizeof...(args)> params;
    params.Write(GetConnectionUserTypes(), args...);
    return DoExecute(query, detail::QueryParameters{params}, statement_cmd_ctl);
  }

  /// Execute statement with stored arguments.
  ///
  /// Suspends coroutine for execution.
  ResultSet Execute(const std::string& statement, const ParameterStore& store) {
    return Execute(OptionalCommandControl{}, statement, store);
  }

  /// Execute statement with stored arguments and per-statement command control.
  ///
  /// Suspends coroutine for execution.
  ResultSet Execute(OptionalCommandControl statement_cmd_ctl,
                    const std::string& statement, const ParameterStore& store);
  /// @}
 private:
  ResultSet DoExecute(const Query& query, const detail::QueryParameters& params,
                      OptionalCommandControl statement_cmd_ctl);
  const UserTypes& GetConnectionUserTypes() const;

  detail::ConnectionPtr conn_;
};

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
