#pragma once

#include <memory>
#include <string>

#include <storages/postgres/options.hpp>
#include <storages/postgres/postgres_fwd.hpp>
#include <storages/postgres/result_set.hpp>

#include <storages/postgres/detail/connection_ptr.hpp>
#include <storages/postgres/detail/query_parameters.hpp>
#include <storages/postgres/detail/time_types.hpp>

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
  /// @throws NotInTransaction, SyntaxError, ConstraintViolation,
  /// InvalidParameterType
  template <typename... Args>
  ResultSet Execute(const std::string& statement, const Args&... args) {
    return Execute(OptionalCommandControl{}, statement, args...);
  }

  /// Execute statement with arbitrary parameters and per-statement command
  /// control.
  ///
  /// Suspends coroutine for execution.
  /// @throws NotInTransaction, SyntaxError, ConstraintViolation,
  /// InvalidParameterType
  template <typename... Args>
  ResultSet Execute(OptionalCommandControl statement_cmd_ctl,
                    const std::string& statement, const Args&... args) {
    detail::QueryParameters params;
    params.Write(GetConnectionUserTypes(), args...);
    return DoExecute(statement, params, statement_cmd_ctl);
  }
  /// @}
 private:
  ResultSet DoExecute(const std::string& statement,
                      const detail::QueryParameters& params,
                      OptionalCommandControl statement_cmd_ctl);
  const UserTypes& GetConnectionUserTypes() const;

 private:
  detail::ConnectionPtr conn_;
};

}  // namespace storages::postgres::detail
