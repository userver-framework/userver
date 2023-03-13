#pragma once

#include <userver/storages/mysql/cluster_host_type.hpp>
#include <userver/storages/mysql/command_result_set.hpp>
#include <userver/storages/mysql/cursor_result_set.hpp>
#include <userver/storages/mysql/options.hpp>
#include <userver/storages/mysql/query.hpp>
#include <userver/storages/mysql/statement_result_set.hpp>

#include <userver/storages/mysql/impl/bind_helper.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

class IClientInterface {
 public:
  /// @brief An alias for `Execute`.
  template <typename... Args>
  StatementResultSet Select(ClusterHostType host_type, const Query& query,
                            const Args&... args) const;

  /// @brief An alias for `Execute`.
  template <typename... Args>
  StatementResultSet Select(OptionalCommandControl command_control,
                            ClusterHostType host_type, const Query& query,
                            const Args&... args) const;

  /// @brief Insert single row into database with default deadline.
  /// `T` is expected to be an aggregate of supported types.
  ///
  /// Basically an alias for Execute(query, AsArgs<T>(row)), where AsArgs is an
  /// imaginary function which passes fields of T as variadic params.
  /// See @ref userver_mysql_types for better understanding of `T` requirements.
  ///
  /// UINVARIANTs on params count missmatch, doesn't validate types.
  template <typename T>
  ExecutionResult InsertOne(const Query& query, const T& row) const;

  /// @brief Insert single row into database with provided CommandControl.
  /// `T` is expected to be an aggregate of supported types.
  ///
  /// Basically an alias for Execute(command_control, ClusterHostType::kMaster,
  /// query, AsArgs<T>(row)), where AsArgs is an imaginary function which passes
  /// fields of T as variadic params.
  /// See @ref userver_mysql_types for better understanding of `T` requirements.
  ///
  /// UINVARIANTs on params count missmatch, doesn't validate types.
  template <typename T>
  ExecutionResult InsertOne(OptionalCommandControl command_control,
                            const Query& query, const T& row) const;

  /// @brief Inserts multiple rows into database with default deadline.
  /// `Container` is expected to be a std::Container of aggregates of
  /// supported types.
  ///
  /// Basically an alias for ExecuteBulk(ClusterHostType::kMaster, query, rows,
  /// throw_on_empty_insert).
  /// See @ref userver_mysql_types for better understanding of
  /// `Container::value_type` requirements.
  ///
  /// @note Requires MariaDB 10.2.6+ as a server
  ///
  /// UINVARIANTs on params count missmatch, doesn't validate types.
  template <typename Container>
  ExecutionResult InsertMany(const Query& query, const Container& rows,
                             bool throw_on_empty_insert = true) const;

  /// @brief Inserts multiple rows into database with provided CommandControl.
  /// `Container` is expected to be a std::Container of aggregates of
  /// supported types.
  ///
  /// Basically an alias for ExecuteBulk(command_control,
  /// ClusterHostType::kMaster, query, rows, throw_on_empty_insert).
  /// See @ref userver_mysql_types for better understanding of
  /// `Container::value_type` requirements.
  ///
  /// @note Requires MariaDB 10.2.6+ as a server
  ///
  /// UINVARIANTs on params count missmatch, doesn't validate types.
  template <typename Container>
  ExecutionResult InsertMany(OptionalCommandControl command_control,
                             const Query& query, const Container& rows,
                             bool throw_on_empty_insert = true) const;

  // TODO : don't require Container to be const, so Convert can move
  // clang-format off
  /// @brief Inserts multiple rows into database with default deadline, on the
  /// flight mapping from `Container::value_type` to `MapTo`.
  /// `Container` is expected to be a std::Container of whatever type pleases
  /// you, `MapTo` is expected to be an aggregate of supported types.
  /// See @ref userver_mysql_types for better understanding of `MapTo` requirements.
  /// You are expected to provide a converter function
  /// `MapTo Convert(const Container::value_type&, storages::mysql::convert::To<MapTo>)`
  /// in namespace of `MapTo` or `storages::mysql::convert`.
  ///
  /// Basically an alias for ExecuteBulkMapped<MapTo>(ClusterHostType::kMaster,
  /// query, rows, throw_on_empty_insert).
  ///
  /// @note Requires MariaDB 10.2.6+ as a server
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
  template <typename MapTo, typename Container>
  ExecutionResult InsertManyMapped(const Query& query, const Container& rows,
                                   bool throw_on_empty_insert = true) const;
  // clang-format on

  // TODO : don't require Container to be const, so Convert can move
  // clang-format off
  /// @brief Inserts multiple rows into database with provided CommandControl, on the
  /// flight remapping from `Container::value_type` to `MapTo`.
  /// `Container` is expected to be a std::Container of whatever type pleases
  /// you, `MapTo` is expected to be an aggregate of supported types.
  /// See @ref userver_mysql_types for better understanding of `MapTo` requirements.
  /// You are expected to provide a converter function
  /// `MapTo Convert(const Container::value_type&, storages::mysql::convert::To<MapTo>)`
  /// in namespace of `MapTo` or storages::mysql::convert.
  ///
  /// Basically an alias for ExecuteBulkMapped<MapTo>(command_control,
  /// ClusterHostType::kMaster, query, rows, throw_on_empty_insert).
  ///
  /// @note Requires MariaDB 10.2.6+ as a server
  ///
  /// UINVARIANTs on params count mismatch, doesn't validate types.
  template <typename MapTo, typename Container>
  ExecutionResult InsertManyMapped(OptionalCommandControl command_control,
                                   const Query& query, const Container& rows,
                                   bool throw_on_empty_insert = true) const;
  // clang-format on

  /// @brief Executes a statement on a host of host_type with default deadline.
  /// Fills placeholders of the statement with args..., `Args` are expected to
  /// be of supported types.
  /// See @ref userver_mysql_types for better understanding of `Args`
  /// requirements.
  ///
  /// UINVARIANTs on params count mismatch doesn't validate types.
  template <typename... Args>
  StatementResultSet Execute(ClusterHostType host_type, const Query& query,
                             const Args&... args) const;

  /// @brief Executes a statement on a host of host_type with provided
  /// CommandControl.
  /// Fills placeholders of the statement with args..., `Args` are expected to
  /// be of supported types.
  /// See @ref userver_mysql_types for better understanding of `Args`
  /// requirements.
  ///
  /// UINVARIANTs on params count mismatch doesn't validate types.
  template <typename... Args>
  StatementResultSet Execute(OptionalCommandControl command_control,
                             ClusterHostType host_type, const Query& query,
                             const Args&... args) const;

  /// @brief Executes a statements on a host of host_type with default deadline.
  /// Fills placeholders of the statements with Container::value_type in a
  /// bulk-manner.
  /// Container is expected to be a std::Container, Container::value_type is
  /// expected to be an aggregate of supported types.
  /// See @ref userver_mysql_types for better understanding of
  /// `Container::value_type` requirements.
  ///
  /// @note Requires MariaDB 10.2.6+ as a server
  ///
  /// UINVARIANTs on params count missmatch, doesn't validate types.
  template <typename Container>
  StatementResultSet ExecuteBulk(ClusterHostType host_type, const Query& query,
                                 const Container& params,
                                 bool throw_on_empty_params = true) const;

  /// @brief Executes a statements on a host of host_type with provided
  /// CommandControl.
  /// Fills placeholders of the statements with
  /// Container::value_type in a bulk-manner.
  /// Container is expected to be a std::Container, Container::value_type is
  /// expected to be an aggregate of supported types.
  /// See @ref userver_mysql_types for better understanding of
  /// `Container::value_type` requirements.
  ///
  /// @note Requires MariaDB 10.2.6+ as a server
  ///
  /// UINVARIANTs on params count missmatch, doesn't validate types.
  template <typename Container>
  StatementResultSet ExecuteBulk(OptionalCommandControl command_control,
                                 ClusterHostType host_type, const Query& query,
                                 const Container& params,
                                 bool throw_on_empty_params = true) const;

  // TODO : don't require Container to be const, so Convert can move
  // clang-format off
  /// @brief Executes a statements on a host of host_type with default deadline,
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
  template <typename MapTo, typename Container>
  StatementResultSet ExecuteBulkMapped(ClusterHostType host_type,
                                       const Query& query,
                                       const Container& params,
                                       bool throw_on_empty_params = true) const;
  // clang-format on

  // TODO : don't require Container to be const, so Convert can move
  // clang-format off
  /// @brief Executes a statements on a host of host_type with provided
  /// CommandControl, on the flight remapping from `Container::value_type`
  /// to `MapTo`.
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
  // clang-format on
  template <typename MapTo, typename Container>
  StatementResultSet ExecuteBulkMapped(OptionalCommandControl command_control,
                                       ClusterHostType host_type,
                                       const Query& query,
                                       const Container& params,
                                       bool throw_on_empty_params = true) const;

 protected:
  ~IClientInterface();

  virtual StatementResultSet DoExecute(
      OptionalCommandControl command_control, ClusterHostType host_type,
      const Query& query, impl::io::ParamsBinderBase& params,
      std::optional<std::size_t> batch_size) const = 0;
};

template <typename... Args>
StatementResultSet IClientInterface::Select(ClusterHostType host_type,
                                            const Query& query,
                                            const Args&... args) const {
  return Select(std::nullopt, host_type, query, args...);
}

template <typename... Args>
StatementResultSet IClientInterface::Select(
    OptionalCommandControl command_control, ClusterHostType host_type,
    const Query& query, const Args&... args) const {
  return Execute(command_control, host_type, query, args...);
}

template <typename T>
ExecutionResult IClientInterface::InsertOne(const Query& query,
                                            const T& row) const {
  return InsertOne(std::nullopt, query, row);
}

template <typename T>
ExecutionResult IClientInterface::InsertOne(
    OptionalCommandControl command_control, const Query& query,
    const T& row) const {
  auto params_binder = impl::BindHelper::BindRowAsParams(row);

  return DoExecute(command_control, ClusterHostType::kMaster, query,
                   params_binder, std::nullopt)
      .AsExecutionResult();
}

template <typename Container>
ExecutionResult IClientInterface::InsertMany(const Query& query,
                                             const Container& rows,
                                             bool throw_on_empty_insert) const {
  return InsertMany(std::nullopt, query, rows, throw_on_empty_insert);
}

template <typename Container>
ExecutionResult IClientInterface::InsertMany(
    OptionalCommandControl command_control, const Query& query,
    const Container& rows, bool throw_on_empty_insert) const {
  return ExecuteBulk(command_control, ClusterHostType::kMaster, query, rows,
                     throw_on_empty_insert)
      .AsExecutionResult();
}

template <typename MapTo, typename Container>
ExecutionResult IClientInterface::InsertManyMapped(
    const Query& query, const Container& rows,
    bool throw_on_empty_insert) const {
  return InsertManyMapped<MapTo>(std::nullopt, query, rows,
                                 throw_on_empty_insert);
}

template <typename MapTo, typename Container>
ExecutionResult IClientInterface::InsertManyMapped(
    OptionalCommandControl command_control, const Query& query,
    const Container& rows, bool throw_on_empty_insert) const {
  return ExecuteBulkMapped<MapTo>(command_control, ClusterHostType::kMaster,
                                  query, rows, throw_on_empty_insert)
      .AsExecutionResult();
}

template <typename... Args>
StatementResultSet IClientInterface::Execute(ClusterHostType host_type,
                                             const Query& query,
                                             const Args&... args) const {
  return Execute(std::nullopt, host_type, query, args...);
}

template <typename... Args>
StatementResultSet IClientInterface::Execute(
    OptionalCommandControl command_control, ClusterHostType host_type,
    const Query& query, const Args&... args) const {
  auto params_binder = impl::BindHelper::BindParams(args...);

  return DoExecute(command_control, host_type, query.GetStatement(),
                   params_binder, std::nullopt);
}

template <typename Container>
StatementResultSet IClientInterface::ExecuteBulk(
    ClusterHostType host_type, const Query& query, const Container& params,
    bool throw_on_empty_params) const {
  return ExecuteBulk(std::nullopt, host_type, query, params,
                     throw_on_empty_params);
}

template <typename Container>
StatementResultSet IClientInterface::ExecuteBulk(
    OptionalCommandControl command_control, ClusterHostType host_type,
    const Query& query, const Container& params,
    [[maybe_unused]] bool throw_on_empty_params) const {
  if (params.empty()) {
    // TODO : what do?
    throw std::runtime_error{"Empty params in bulk execution"};
    /*if (throw_on_empty_params) {
      throw std::runtime_error{"Empty params in bulk execution"};
    } else {
      return;
    }*/
  }

  auto params_binder = impl::BindHelper::BindContainerAsParams(params);

  return DoExecute(command_control, host_type, query.GetStatement(),
                   params_binder, std::nullopt);
}

template <typename MapTo, typename Container>
StatementResultSet IClientInterface::ExecuteBulkMapped(
    ClusterHostType host_type, const Query& query, const Container& params,
    bool throw_on_empty_params) const {
  return ExecuteBulkMapped<MapTo>(std::nullopt, host_type, query, params,
                                  throw_on_empty_params);
}

template <typename MapTo, typename Container>
StatementResultSet IClientInterface::ExecuteBulkMapped(
    OptionalCommandControl command_control, ClusterHostType host_type,
    const Query& query, const Container& params,
    [[maybe_unused]] bool throw_on_empty_params) const {
  if (params.empty()) {
    // TODO : what do?
    throw std::runtime_error{"Empty params in bulk execution"};
    /*if (throw_on_empty_params) {
      throw std::runtime_error{"Empty params in bulk execution"};
    } else {
      return;
    }*/
  }

  auto params_binder =
      impl::BindHelper::BindContainerAsParamsMapped<MapTo>(params);

  return DoExecute(command_control, host_type, query.GetStatement(),
                   params_binder, std::nullopt);
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
