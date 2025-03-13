#include <userver/storages/sqlite/impl/result_wrapper.hpp>

#include <userver/utils/assert.hpp>

#include <userver/storages/sqlite/impl/statements.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

ResultWrapper::ResultWrapper(StatementBasePtr prepare_statement)
    : prepare_statement_(std::move(prepare_statement)) {}

ResultWrapper::~ResultWrapper() = default;

int ResultWrapper::RowsAffected() const noexcept {
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

template <>
int32_t ResultWrapper::GetColumn<int32_t>(int column) {
  return prepare_statement_->GetInt32Column(column);
}

template <>
uint32_t ResultWrapper::GetColumn<uint32_t>(int column) {
  return prepare_statement_->GetUInt32Column(column);
}

template <>
int64_t ResultWrapper::GetColumn<int64_t>(int column) {
  return prepare_statement_->GetInt64Column(column);
}

template <>
double ResultWrapper::GetColumn<double>(int column) {
  return prepare_statement_->GetDoubleColumn(column);
}

template <>
const char* ResultWrapper::GetColumn<const char*>(int column) {
  return prepare_statement_->GetCStringColumn(column);
}

template <>
const void* ResultWrapper::GetColumn<const void*>(int column) {
  return prepare_statement_->GetBlobColumn(column);
}

template <>
std::string ResultWrapper::GetColumn<std::string>(int column) {
  return prepare_statement_->GetStringColumn(column);
}

template <>
std::vector<uint8_t> ResultWrapper::GetColumn<std::vector<uint8_t>>(
    int column) {
  return prepare_statement_->GetBytesColumn(column);
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
