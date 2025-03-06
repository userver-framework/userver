#include <userver/storages/sqlite/impl/result_wrapper.hpp>

#include <userver/storages/sqlite/impl/statements.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

ResultWrapper::ResultWrapper(std::shared_ptr<StatementBase> prepare_statement)
    : prepare_statement_(std::move(prepare_statement)) {}

ResultWrapper::~ResultWrapper() = default;

int ResultWrapper::RowsAffected() const noexcept {
  // TODO: need check on SQLITE_DONE?
  // TODO: is this an CPU bound operation? does it need to be run on
  // blocking_task_processor_?
  return prepare_statement_->RowsAffected();
}

int ResultWrapper::LastInsertRowId() const noexcept {
  return prepare_statement_->LastInsertRowId();
}

bool ResultWrapper::HasNext() const noexcept {
  return prepare_statement_->HasNext();
}

bool ResultWrapper::IsDone() const noexcept {
  return prepare_statement_->IsDone();
}

void ResultWrapper::Next() noexcept { prepare_statement_->Next(); }

int ResultWrapper::ColumnCount() const noexcept {
  return prepare_statement_->ColumnCount();
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
