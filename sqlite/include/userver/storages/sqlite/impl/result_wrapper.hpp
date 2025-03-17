#pragma once

#include <boost/pfr.hpp>

#include <userver/engine/task/task_processor_fwd.hpp>

#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/impl/extractor_base.hpp>
#include <userver/storages/sqlite/row_types.hpp>
#include <userver/storages/sqlite/sqlite_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

/// @brief Result wrapper and fetch helper
class ResultWrapper final {
 public:
  ResultWrapper(StatementBasePtr prepare_statement,
                engine::TaskProcessor& blocking_task_processor);
  ~ResultWrapper();

  StatementBasePtr GetStatement() noexcept;

  void FetchResult(impl::ExtractorBase& extractor);

  ExecutionResult GetExecutionResult() noexcept;

 private:
  void ExecutionStep();

  StatementBasePtr prepare_statement_;
  engine::TaskProcessor& blocking_task_processor_;
};

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
