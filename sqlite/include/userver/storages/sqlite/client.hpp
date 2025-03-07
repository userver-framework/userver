#pragma once

/// @file userver/storages/sqlite/connection.hpp
/// @copybrief @copybrief storages::sqlite::Connection

#include <memory>
#include <optional>

#include <userver/engine/task/task_processor_fwd.hpp>

#include <userver/storages/sqlite/impl/connection.hpp>  // TODO: remove heavy include
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>
#include <userver/storages/sqlite/savepoint.hpp>
#include <userver/storages/sqlite/transaction.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

class Client;
using ClientPtr = std::shared_ptr<Client>;

namespace infra {
class Pool;
using PoolPtr = std::shared_ptr<Pool>;

namespace strategy {
class PoolStrategyBase;
using PoolStrategyBasePtr = std::unique_ptr<PoolStrategyBase>;
}  // namespace strategy

}  // namespace infra

/// @ingroup userver_clients
///
/// @brief Client interface for a SQLite connection.
/// Usually retrieved from components::SQLite
class Client final {
 public:
  /// @brief Connection constructor
  Client(const settings::SQLiteSettings& settings,
         engine::TaskProcessor& blocking_task_processor);
  /// @brief Connection destructor
  ~Client();

  template <typename... Args>
  ResultSet Execute(const Query& query, const Args&... args) const;

  template <typename... Args>
  ResultSet Execute(settings::OptionalCommandControl optional_cc,
                    const Query& query, const Args&... args) const;

  template <typename T>
  ResultSet ExecuteDecompose(const Query& query, const T& row) const;

  template <typename T>
  ResultSet ExecuteDecompose(settings::OptionalCommandControl optional_cc,
                             const Query& query, const T& row) const;

  // like
  // https://docs.python.org/3/library/sqlite3.html#sqlite3.Cursor.executemany
  template <typename Container>
  void ExecuteMany(const Query& query, const Container& params) const;

  template <typename Container>
  void ExecuteMany(settings::OptionalCommandControl optional_cc,
                   const Query& query, const Container& params) const;

  Transaction Begin(std::string name,
                    const settings::TransactionOptions&) const;

  Transaction Begin(settings::OptionalCommandControl optional_cc,
                    std::string name,
                    const settings::TransactionOptions&) const;

  Savepoint Save(std::string name) const;

  Savepoint Save(settings::OptionalCommandControl optional_cc,
                 std::string name) const;

  infra::ConnectionPtr GetConnection(
      settings::CommandControl::OperationType op_type) const;

 private:
  template <typename... Args>
  ResultSet DoExecute(settings::OptionalCommandControl optional_cc,
                      const Query& query, const Args&... args) const;

  infra::strategy::PoolStrategyBasePtr pool_strategy_;
};

template <typename... Args>
ResultSet Client::Execute(const Query& query, const Args&... args) const {
  return Execute(std::nullopt, query, args...);
}

template <typename... Args>
ResultSet Client::Execute(settings::OptionalCommandControl optional_cc,
                          const Query& query, const Args&... args) const {
  return DoExecute(optional_cc, query, args...);
}

template <typename... Args>
ResultSet Client::DoExecute(settings::OptionalCommandControl optional_cc,
                            const Query& query, const Args&... args) const {
  if (!optional_cc.has_value()) {
    optional_cc = settings::CommandControl::GetDefault();
  }
  auto connection = GetConnection(optional_cc->operation_type);
  return connection->ExecuteCommand(optional_cc, query, args...);
}

template <typename T>
ResultSet Client::ExecuteDecompose(const Query& query, const T& row) const {
  return ExecuteDecompose(std::nullopt, query, row);
}

template <typename T>
ResultSet Client::ExecuteDecompose(settings::OptionalCommandControl optional_cc,
                                   const Query& query, const T& row) const {
  if (!optional_cc.has_value()) {
    optional_cc = settings::CommandControl::GetDefault();
  }
  auto connection = GetConnection(optional_cc->operation_type);
  return connection->ExecuteDecompose(optional_cc, query, row);
}

template <typename Container>
void Client::ExecuteMany(const Query& query, const Container& params) const {
  return ExecuteMany(std::nullopt, query, params);
}

template <typename Container>
void Client::ExecuteMany(settings::OptionalCommandControl optional_cc,
                         const Query& query, const Container& params) const {
  if (!optional_cc.has_value()) {
    optional_cc = settings::CommandControl::GetDefault();
  }
  auto connection = GetConnection(optional_cc->operation_type);
  return connection->ExecuteMany(optional_cc, query, params);
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
