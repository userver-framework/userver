#pragma once

/// @file userver/storages/mysql/transaction.hpp

#include <userver/tracing/span.hpp>
#include <userver/utils/fast_pimpl.hpp>

#include <userver/storages/mysql/cluster_host_type.hpp>
#include <userver/storages/mysql/command_result_set.hpp>
#include <userver/storages/mysql/cursor_result_set.hpp>
#include <userver/storages/mysql/impl/bind_helper.hpp>
#include <userver/storages/mysql/options.hpp>
#include <userver/storages/mysql/query.hpp>
#include <userver/storages/mysql/statement_result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace infra {
class ConnectionPtr;
}

/// @brief RAII transaction wrapper, auto-<b>ROLLBACK</b>s on destruction if no
/// prior `Commit`/`Rollback` call was made.
///
/// This type can't be constructed in user code and is always retrieved from
/// storages::mysql::Cluster
class Transaction final {
 public:
  explicit Transaction(infra::ConnectionPtr&& connection,
                       engine::Deadline deadline);
  ~Transaction();
  Transaction(const Transaction& other) = delete;
  Transaction(Transaction&& other) noexcept;

  /// @brief Executes a statement with default deadline.
  /// Fills placeholders of the statement with args..., `Args` are expected to
  /// be of supported types.
  /// See @ref userver_mysql_types for better understanding of `Args`
  /// requirements.
  ///
  /// UINVARIANTs on params count mismatch doesn't validate types.
  template <typename... Args>
  StatementResultSet Execute(const Query& query, const Args&... args) const;

  /// @brief Executes a statement with default deadline.
  ///
  /// Basically an alias for Execute(host_type, query, AsArgs<T>(row)),
  /// where AsArgs is an imaginary function which passes fields of T as
  /// variadic params. Handy for one-liner inserts.
  /// See @ref userver_mysql_types for better understanding of `T` requirements.
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
  template <typename T>
  StatementResultSet ExecuteDecompose(const Query& query, const T& row) const;

  /// @brief Executes a statement with default deadline.
  /// Fills placeholders of the statements with Container::value_type in a
  /// bulk-manner.
  /// Container is expected to be a std::Container, Container::value_type is
  /// expected to be an aggregate of supported types.
  /// See @ref userver_mysql_types for better understanding of
  /// `Container::value_type` requirements.
  ///
  /// @note Requires MariaDB 10.2.6+ as a server
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
  /// UINVARIANTs on empty params container.
  template <typename Container>
  StatementResultSet ExecuteBulk(const Query& query,
                                 const Container& params) const;

  // TODO : don't require Container to be const, so Convert can move
  // clang-format off
  /// @brief Executes a statement with default deadline,
  /// on the flight remapping from `Container::value_type` to `MapTo`.
  /// `Container` is expected to be a std::Container of whatever type pleases
  /// you, `MapTo` is expected to be an aggregate of supported types.
  /// See @ref userver_mysql_types for better understanding of `MapTo` requirements.
  /// You are expected to provide a converter function
  /// `MapTo Convert(const Container::value_type&, storages::mysql::convert::To<MapTo>)`
  /// in namespace of `MapTo` or storages::mysql::convert.
  ///
  /// @note Requires MariaDB 10.2.6+ as a server
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
  /// UINVARIANTs on empty params container.
  template <typename MapTo, typename Container>
  StatementResultSet ExecuteBulkMapped(const Query& query,
                                       const Container& params) const;
  // clang-format on

  /// @brief Commit the transaction
  void Commit();

  /// @brief Rollback the transaction
  void Rollback();

 private:
  StatementResultSet DoExecute(const Query& query,
                               impl::io::ParamsBinderBase& params) const;

  void AssertValid() const;

  utils::FastPimpl<infra::ConnectionPtr, 24, 8> connection_;
  engine::Deadline deadline_;
  tracing::Span span_;
};

template <typename... Args>
StatementResultSet Transaction::Execute(const Query& query,
                                        const Args&... args) const {
  auto params_binder = impl::BindHelper::BindParams(args...);

  return DoExecute(query.GetStatement(), params_binder);
}

template <typename T>
StatementResultSet Transaction::ExecuteDecompose(const Query& query,
                                                 const T& row) const {
  auto params_binder = impl::BindHelper::BindRowAsParams(row);

  return DoExecute(query, params_binder);
}

template <typename Container>
StatementResultSet Transaction::ExecuteBulk(const Query& query,
                                            const Container& params) const {
  UINVARIANT(!params.empty(), "Empty params in bulk execution");

  auto params_binder = impl::BindHelper::BindContainerAsParams(params);

  return DoExecute(query.GetStatement(), params_binder);
}

template <typename MapTo, typename Container>
StatementResultSet Transaction::ExecuteBulkMapped(
    const Query& query, const Container& params) const {
  UINVARIANT(!params.empty(), "Empty params in bulk execution");

  auto params_binder =
      impl::BindHelper::BindContainerAsParamsMapped<MapTo>(params);

  return DoExecute(query.GetStatement(), params_binder);
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
