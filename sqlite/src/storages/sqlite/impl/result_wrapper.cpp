#include <userver/storages/sqlite/impl/result_wrapper.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

ResultWrapper::ResultWrapper(std::shared_ptr<FieldExtractorBase> fieldExtractor,
                             std::shared_ptr<sqlite3_stmt> stmt,
                             int exec_status)
    : ResultWrapperBase(fieldExtractor),
      stmt_(std::move(stmt)),
      exec_status_(exec_status) {}

ResultWrapper::~ResultWrapper() = default;

int ResultWrapper::RowsAffected() const noexcept {
  // TODO: need check on SQLITE_DONE?
  // TODO: is this an CPU bound operation? does it need to be run on
  // blocking_task_processor_?
  return sqlite3_changes(sqlite3_db_handle(stmt_.get()));
}

int ResultWrapper::LastInsertRowId() const noexcept {
  return sqlite3_last_insert_rowid(sqlite3_db_handle(stmt_.get()));
}

bool ResultWrapper::HasNext() const noexcept {
  return exec_status_ == SQLITE_ROW;
}

bool ResultWrapper::IsDone() const noexcept {
  return exec_status_ == SQLITE_DONE;
}

void ResultWrapper::Next() noexcept {
  exec_status_ = sqlite3_step(stmt_.get());
  if (IsDone()) {
    sqlite3_reset(stmt_.get());
  }
}

int ResultWrapper::ColumnCount() const noexcept {
  return sqlite3_column_count(stmt_.get());
}

int32_t FieldExtractor::GetInt32Column(int column) const noexcept {
  // TODO: is this an CPU bound operation? does it need to be run on
  // blocking_task_processor_?
  // TODO: Check for null, what to return if the value is null, not 0
  return sqlite3_column_int(stmt_.get(), column);
}

uint32_t FieldExtractor::GetUInt32Column(int column) const noexcept {
  return GetInt64Column(column);
}

int64_t FieldExtractor::GetInt64Column(int column) const noexcept {
  return sqlite3_column_int64(stmt_.get(), column);
}

double FieldExtractor::GetDoubleColumn(int column) const noexcept {
  return sqlite3_column_double(stmt_.get(), column);
}

const char* FieldExtractor::GetCStringColumn(int column) const noexcept {
  // Return a pointer to the text value (NULL terminated string) of the column
  // specified by its index starting at 0
  auto text_ptr =
      reinterpret_cast<const char*>(sqlite3_column_text(stmt_.get(), column));
  return text_ptr ? text_ptr : "";
}

const void* FieldExtractor::GetBlobColumn(int column) const noexcept {
  return sqlite3_column_blob(stmt_.get(), column);
}

std::string FieldExtractor::GetStringColumn(int column) const noexcept {
  // Note: using sqlite3_column_blob and not sqlite3_column_text
  // - no need for sqlite3_column_text to add a \0 on the end, as we're getting
  // the bytes length directly
  //   however, we need to call sqlite3_column_bytes() to ensure correct format.
  //   It's a noop on a BLOB or a TEXT value with the correct encoding (UTF-8).
  //   Otherwise it'll do a conversion to TEXT (UTF-8).
  // (void)sqlite3_column_bytes(stmt, column);
  auto data = static_cast<const char*>(GetBlobColumn(column));
  // SQLite docs: "The safest policy is to invoke… sqlite3_column_blob()
  // followed by sqlite3_column_bytes()"
  // Note: std::string is ok to pass nullptr as first arg, if length is 0
  return std::string(data, sqlite3_column_bytes(stmt_.get(), column));
}

std::vector<uint8_t> FieldExtractor::GetBytesColumn(int column) const noexcept {
  const void* blob = GetBlobColumn(column);
  int size = sqlite3_column_bytes(stmt_.get(), column);
  return blob ? std::vector<uint8_t>(static_cast<const uint8_t*>(blob),
                                     static_cast<const uint8_t*>(blob) + size)
              : std::vector<uint8_t>{};
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
