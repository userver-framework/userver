#pragma once

/// @file userver/storages/sqlite/connection.hpp
/// @copybrief @copybrief storages::sqlite::Connection

#include <memory>

#include <userver/components/component_fwd.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/storages/sqlite/impl/statements_cache.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>
#include <userver/storages/sqlite/transaction.hpp>
#include "userver/storages/sqlite/impl/connection_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

class Connection;
using ConnectionPtr = std::shared_ptr<Connection>;

/// @ingroup userver_clients
///
/// @brief Client interface for a SQLite connection.
/// Usually retrieved from components::SQLite
class Connection final {
 public:
  /// @brief Connection constructor
  Connection(const SQLiteSettings& settings,
             engine::TaskProcessor& blocking_task_processor);
  /// @brief Connection destructor
  ~Connection();

  template <typename... Args>
  ResultSet Execute(const Query& query, const Args&... args) const;

  template <typename... Args>
  ResultSet Execute(OptionalCommandControl optional_cc, const Query& query,
                    const Args&... args) const;

  // TODO: Implement the binding of the structure/tuple in a set of parameters
  // for request template <typename T> ResultSet ExecuteDecompose(const Query&
  // query,
  //                            const T& row [[maybe_unused]]) const;

  // template <typename T>
  // ResultSet ExecuteDecompose(OptionalCommandControl optional_cc,
  //                            const Query& query, const T& row) const;

  // TODO: ExecuteBulk will work similarly to executemany in Python, that is,
  // a consistent call of ordinary execute
  // https://docs.python.org/3/library/sqlite3.html#sqlite3.Cursor.executemany

  // template <typename Container>
  // ResultSet ExecuteBulk(const Query& query, const Container& params) const;

  // template <typename Container>
  // ResultSet ExecuteBulk(OptionalCommandControl optional_cc, const Query&
  // query,
  //                       const Container& params) const;

  Transaction Begin(std::string name, const TransactionOptions&);

  Transaction Begin(OptionalCommandControl optional_cc, std::string name,
                    const TransactionOptions&);

 private:
  template <typename... Args>
  ResultSet DoExecute(OptionalCommandControl optional_cc, const Query& query,
                      std::optional<std::size_t> batch_size,
                      const Args&... args) const;

  // TODO: maybe it is better to use a class of an index for implementation or
  // fastpimpl
  std::shared_ptr<impl::ConnectionImpl> pimpl_;
};

template <typename... Args>
ResultSet Connection::Execute(const Query& query, const Args&... args) const {
  return Execute(std::nullopt, query, args...);
}

template <typename... Args>
ResultSet Connection::Execute(OptionalCommandControl optional_cc
                              [[maybe_unused]],
                              const Query& query,
                              const Args&... args [[maybe_unused]]) const {
  return DoExecute(optional_cc, query, std::nullopt, args...);
}

template <typename... Args>
ResultSet Connection::DoExecute(OptionalCommandControl command_control
                                [[maybe_unused]],
                                const Query& query [[maybe_unused]],
                                std::optional<std::size_t> batch_size
                                [[maybe_unused]],
                                const Args&... args [[maybe_unused]]) const {
  return pimpl_->ExecuteCommand(command_control, query, args...);
}

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
