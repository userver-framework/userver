#include <userver/storages/sqlite/impl/result_wrapper.hpp>

#include <userver/storages/sqlite/execution_result.hpp>
#include <userver/storages/sqlite/impl/connection.hpp>
#include <userver/storages/sqlite/impl/statement_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

ResultWrapper::ResultWrapper(
    StatementBasePtr prepare_statement,
    std::shared_ptr<infra::ConnectionPtr> connection_ptr)
    : prepare_statement_{std::move(prepare_statement)},
      connection_ptr_{std::move(connection_ptr)} {
  ExecutionStep();  // First execution step
}

ResultWrapper::~ResultWrapper() = default;

StatementBasePtr ResultWrapper::GetStatement() noexcept {
  return prepare_statement_;
}

void ResultWrapper::FetchAllResult(impl::ExtractorBase& extractor) {
  while (prepare_statement_->HasNext()) {
    extractor.BindNextRow();
    ExecutionStep();  // blocking IO
  }
}

bool ResultWrapper::FetchResult(impl::ExtractorBase& extractor,
                                size_t batch_size) {
  for (size_t i = 0; i < batch_size; ++i) {
    if (!prepare_statement_->HasNext()) {
      return false;
    }
    extractor.BindNextRow();
    ExecutionStep();  // blocking IO
  }
  return prepare_statement_->HasNext();
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
  if (connection_ptr_) {
    (*connection_ptr_)->ExecutionStep(prepare_statement_);
  }
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
