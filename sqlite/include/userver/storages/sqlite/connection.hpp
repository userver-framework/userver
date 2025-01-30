#pragma once

/// @file userver/storages/sqlite/connection.hpp
/// @copybrief @copybrief storages::sqlite::Connection

#include <memory>

#include <sqlite3.h>

#include <userver/components/component_fwd.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/storages/sqlite/impl/statements_cache.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>
#include <userver/storages/sqlite/transaction.hpp>

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

  // template <typename T>
  // ResultSet ExecuteDecompose(const Query& query,
  //                            const T& row [[maybe_unused]]) const;

  // template <typename T>
  // ResultSet ExecuteDecompose(OptionalCommandControl optional_cc,
  //                            const Query& query, const T& row) const;

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
  struct SQLiteHandlerDeleter {
    void operator()(sqlite3* sqlite_handle);
  };

  using NativeHandlerPtr = std::unique_ptr<sqlite3, SQLiteHandlerDeleter>;

  sqlite3* getHandle() const noexcept;

  sqlite3* OpenDatabase(const SQLiteSettings& settings) const;

  template <typename... Args>
  ResultSet DoExecute(OptionalCommandControl optional_cc, const Query& query,
                      std::optional<std::size_t> batch_size,
                      const Args&... args) const;

  engine::TaskProcessor& blocking_task_processor_;
  NativeHandlerPtr db_handler_;
  impl::StatementsCache statements_cache_;
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
  // TODO: Add support of args like WHERE key = ?, (?, ?, ?)
  return DoExecute(optional_cc, query, std::nullopt, args...);
}

template <typename... Args>
ResultSet Connection::DoExecute(OptionalCommandControl command_control
                                [[maybe_unused]],
                                const Query& query [[maybe_unused]],
                                std::optional<std::size_t> batch_size
                                [[maybe_unused]],
                                const Args&... args [[maybe_unused]]) const {
  // Prepare statement and execute first step
  // TODO: For simple INSERT, DELETE, UPDATE this works, but for example using
  // RETURNING clauses, obviously repeated calls to sqlite3_step are required to
  // get all rows https://www.sqlite.org/lang_returning.html
  // Based on circumstantial evidence, nested and complex DML queries execute in
  // one sqlite3_step, but this requires inspection and profiling
  return engine::AsyncNoSpan(blocking_task_processor_,
                             [this, query, args...] {
                               auto stmt = statements_cache_.PrepareStatement(
                                   query.GetStatement());
                               return stmt->Execute(args...);
                             })
      .Get();
}

// template <typename T>
// ResultSet Connection::ExecuteDecompose(const Query& query,
//                                        const T& row [[maybe_unused]]) const {
//   return DoExecute(std::nullopt, query.GetStatement(), std::nullopt);
// }

// template <typename T>
// ResultSet Connection::ExecuteDecompose(OptionalCommandControl optional_cc
//                                        [[maybe_unused]],
//                                        const Query& query,
//                                        const T& row [[maybe_unused]]) const {
//   return DoExecute(optional_cc, query.GetStatement(), std::nullopt);
// }

// // Is this relevant or not?
// template <typename Container>
// ResultSet Connection::ExecuteBulk(const Query& query,
//                                   const Container& params) const {
//   return ExecuteBulk(std::nullopt, query, params);
// }

// template <typename Container>
// ResultSet Connection::ExecuteBulk(OptionalCommandControl optional_cc,
//                                   const Query& query,
//                                   const Container& params
//                                   [[maybe_unused]]) const {
//   return DoExecute(optional_cc, query, std::nullopt);
// }

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
