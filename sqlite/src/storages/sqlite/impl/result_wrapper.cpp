#include <userver/storages/sqlite/impl/result_wrapper.hpp>

#include <userver/engine/async.hpp>

#include <userver/storages/sqlite/execution_result.hpp>
#include <userver/storages/sqlite/impl/statement_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

ResultWrapper::ResultWrapper(StatementBasePtr prepare_statement,
                             engine::TaskProcessor& blocking_task_processor_)
    : prepare_statement_{std::move(prepare_statement)},
      blocking_task_processor_(blocking_task_processor_) {
  ExecutionStep();
}

ResultWrapper::~ResultWrapper() = default;

StatementBasePtr ResultWrapper::GetStatement() noexcept {
  return prepare_statement_;
}

void ResultWrapper::FetchResult(impl::ExtractorBase& extractor) {
  while (prepare_statement_->HasNext()) {
    extractor.BindNextRow();
    ExecutionStep();  // blocking IO
  }
}

ExecutionResult ResultWrapper::GetExecutionResult() noexcept {
  const int rows_affected = prepare_statement_->RowsAffected();
  const int last_insert_id = prepare_statement_->LastInsertRowId();

  ExecutionResult result{};
  result.rows_affected = rows_affected;
  result.last_insert_id = last_insert_id;
  return result;
}

void ResultWrapper::ExecutionStep() {
  engine::AsyncNoSpan(blocking_task_processor_, [this] {
    prepare_statement_->Next();
  }).Get();
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
