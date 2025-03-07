#pragma once

#include <memory>

#include <sqlite3.h>

#include <userver/components/component_fwd.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

#include <userver/storages/sqlite/impl/result_wrapper.hpp>
#include <userver/storages/sqlite/impl/statements.hpp>
#include <userver/storages/sqlite/impl/statements_cache.hpp>
#include <userver/storages/sqlite/options.hpp>
#include <userver/storages/sqlite/query.hpp>
#include <userver/storages/sqlite/result_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

class Connection {
 public:
  Connection(const settings::SQLiteSettings& settings,
             engine::TaskProcessor& blocking_task_processor);

  ~Connection();

  settings::ConnectionSettings const& GetSettings() const noexcept;
  sqlite3* GetHandle() const noexcept;

  ResultSet ExecuteCommand(settings::OptionalCommandControl optional_cc,
                           impl::StatementPtr prepare_statement) const;

  void Begin(const settings::TransactionOptions& options);

  void Commit();

  void Rollback();

  void Savepoint(const std::string& name);

  void Release(const std::string& name);

  void RollbackTo(const std::string& name);

  std::string PrepareString(const std::string& str);

  StatementPtr PrepareStatement(const Query& query);

  bool IsBroken() const;

  // There are places (destructors, basically) where we want to run some
  // function even if connection is already broken, because that function frees
  // resources no matter what. Can't use BrokenGuard for that, 'cause it will
  // throw on construction, but still need a way no notify a connection that it
  // broke.
  void NotifyBroken();

 private:
  struct SQLiteHandlerDeleter {
    void operator()(sqlite3* sqlite_handle);
  };

  using NativeHandlerPtr = std::unique_ptr<sqlite3, SQLiteHandlerDeleter>;

  sqlite3* OpenDatabase(const settings::SQLiteSettings& settings);

  StatementPtr MakeStatement(const std::string& query) const;

  template <typename... Args>
  ResultSet ExecuteCommandNoPrepare(const std::string& query,
                                    const Args&... args) const;

  engine::TaskProcessor& blocking_task_processor_;
  NativeHandlerPtr db_handler_;
  settings::ConnectionSettings connection_settings_;
  impl::StatementsCache statements_cache_;
  std::atomic<bool> broken_{false};
};

template <typename... Args>
ResultSet Connection::ExecuteCommandNoPrepare(const std::string& query,
                                              const Args&... args) const {
  auto statement = MakeStatement(query);
  statement->UpdateParamsBindings(args...);
  return ExecuteCommand(std::nullopt, statement);
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
