#pragma once

#include <userver/engine/task/task_processor_fwd.hpp>

#include <userver/storages/sqlite/impl/native_handler.hpp>
#include <userver/storages/sqlite/impl/result_wrapper.hpp>
#include <userver/storages/sqlite/impl/statement.hpp>
#include <userver/storages/sqlite/impl/statements_cache.hpp>
#include <userver/storages/sqlite/infra/connection_ptr.hpp>
#include <userver/storages/sqlite/infra/statistics/statistics_counter.hpp>
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

  StatementPtr PrepareStatement(const Query& query);

  void ExecutionStep(StatementBasePtr prepare_statement) const;

  void Begin(const settings::TransactionOptions& options);
  void Commit();
  void Rollback();

  void Save(const std::string& name);
  void Release(const std::string& name);
  void RollbackTo(const std::string& name);

  infra::statistics::CountExecute GetStatisticsCounter() const;

  bool IsBroken() const;
  void NotifyBroken();

 private:
  void ExecuteQuery(const std::string& query) const;
  void SetSettings(const settings::SQLiteSettings& settings);

  engine::TaskProcessor& blocking_task_processor_;
  impl::NativeHandler db_handler_;
  settings::SQLiteSettings settings_;
  impl::StatementsCache statements_cache_;
  std::atomic<bool> broken_{false};
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
