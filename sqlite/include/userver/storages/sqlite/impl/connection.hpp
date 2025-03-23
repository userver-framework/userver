#pragma once

#include <userver/engine/task/task_processor_fwd.hpp>

#include <userver/storages/sqlite/impl/native_handler.hpp>
#include <userver/storages/sqlite/impl/result_wrapper.hpp>
#include <userver/storages/sqlite/impl/statement.hpp>
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

  ResultSet ExecuteCommand(impl::StatementBasePtr prepare_statement) const;

  void Begin(const settings::TransactionOptions& options);

  void Commit();

  void Rollback();

  void Savepoint(const std::string& name);

  void Release(const std::string& name);

  void RollbackTo(const std::string& name);

  std::string PrepareString(const std::string& str);

  StatementPtr PrepareStatement(const Query& query);

  bool IsBroken() const;

  void NotifyBroken();

 private:
  template <typename... Args>
  ResultSet ExecuteCommandNoPrepare(const std::string& query,
                                    const Args&... args) const;

  void ExecuteQuery(const std::string& query) const;

  void SetSettings(const settings::SQLiteSettings& settings);

  engine::TaskProcessor& blocking_task_processor_;
  impl::NativeHandler db_handler_;
  settings::ConnectionSettings connection_settings_;
  impl::StatementsCache statements_cache_;
  std::atomic<bool> broken_{false};
};

template <typename... Args>
ResultSet Connection::ExecuteCommandNoPrepare(const std::string& query,
                                              const Args&... args) const {
  auto statement = std::make_shared<Statement>(db_handler_, query);
  statement->UpdateParamsBindings(args...);
  return ExecuteCommand(statement);
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
