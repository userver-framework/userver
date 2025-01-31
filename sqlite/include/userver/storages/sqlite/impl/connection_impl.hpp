#pragma once

#include <sqlite3.h>

#include <memory>
#include <userver/components/component_fwd.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/storages/sqlite/impl/statements_cache.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>
#include "userver/storages/sqlite/impl/statements.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class ConnectionImpl {
 public:
  ConnectionImpl(const SQLiteSettings& settings,
                 engine::TaskProcessor& blocking_task_processor);

  ConnectionSettings const& GetSettings() const noexcept;
  sqlite3* GetHandle() const noexcept;

  template <typename... Args>
  ResultSet ExecuteCommand(OptionalCommandControl optional_cc,
                           const Query& query, const Args&... args) const;

  void Commit();
  void Rollback();

 private:
  struct SQLiteHandlerDeleter {
    void operator()(sqlite3* sqlite_handle);
  };

  using NativeHandlerPtr = std::unique_ptr<sqlite3, SQLiteHandlerDeleter>;

  sqlite3* OpenDatabase(const SQLiteSettings& settings) const;

  std::shared_ptr<Statement> MakeStatement(const std::string& statement) const;

  template <typename... Args>
  ResultSet ExecuteCommand(const Query& query, const Args&... args) const;

  template <typename... Args>
  ResultSet ExecuteCommandNoPrepare(const Query& query,
                                    const Args&... args) const;

  engine::TaskProcessor& blocking_task_processor_;
  ConnectionSettings settings_;
  NativeHandlerPtr db_handler_;
  impl::StatementsCache statements_cache_;
  bool transaction_commited_ = false;  // TODO: Is it safe?
};

template <typename... Args>
ResultSet ConnectionImpl::ExecuteCommand(OptionalCommandControl optional_cc
                                         [[maybe_unused]],
                                         const Query& query,
                                         const Args&... args) const {
  // TODO Process optional_cc
  if (settings_.prepared_statements ==
      ConnectionSettings::kNoPreparedStatements) {
    return ExecuteCommandNoPrepare(query, args...);
  }
  return ExecuteCommand(query, args...);
}

template <typename... Args>
ResultSet ConnectionImpl::ExecuteCommand(const Query& query,
                                         const Args&... args) const {
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

template <typename... Args>
ResultSet ConnectionImpl::ExecuteCommandNoPrepare(const Query& query,
                                                  const Args&... args) const {
  return engine::AsyncNoSpan(blocking_task_processor_,
                             [this, query, args...] {
                               auto stmt = MakeStatement(query.GetStatement());
                               return stmt->Execute(args...);
                             })
      .Get();
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
