#include <string_view>
#include <userver/storages/sqlite/impl/statement.hpp>

#include <fmt/format.h>

#include <userver/storages/sqlite/exceptions.hpp>
#include <userver/storages/sqlite/impl/sqlite3_include.hpp>
#include <userver/storages/sqlite/operation_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

Statement::Statement(const NativeHandler& db_handler, const std::string& statement)
    : db_handler_{db_handler},
      prepare_statement_(prepareStatement(statement)),
      column_count_(sqlite3_column_count(prepare_statement_.get())) {}

Statement::~Statement() = default;

Statement::Statement(Statement&& other) noexcept = default;

void Statement::SQLiteStatementDeleter::operator()(sqlite3_stmt* stmt) {
    // It's return last execution error status, we don't need to check it here
    sqlite3_finalize(stmt);
}

std::string Statement::GetStatementText() const noexcept {
    const char* query = sqlite3_sql(prepare_statement_.get());
    if (!query) {
        return std::string{};
    }
    std::string queryString{query};
    return queryString;
}

std::string Statement::getExpandedStatementText() const noexcept {
    char* expanded = sqlite3_expanded_sql(prepare_statement_.get());
    if (!expanded) {
        return std::string{};
    }
    std::string expandedString{expanded};
    sqlite3_free(expanded);
    return expandedString;
}

OperationType Statement::GetOperationType() const noexcept {
    // In rare occasions misses
    // https://www.sqlite.org/c3ref/stmt_readonly.html
    // TODO: Can we use it to determine correct pool to execute query
    return sqlite3_stmt_readonly(prepare_statement_.get()) ? OperationType::kReadOnly : OperationType::kReadWrite;
}

Statement::NativeStatementPtr Statement::prepareStatement(const std::string& statement_str) {
    sqlite3_stmt* statement = nullptr;
    // TODO: It can indirectly triggers I/O?
    const int ret_code = sqlite3_prepare_v2(
        db_handler_.GetHandle(), statement_str.c_str(), static_cast<int>(statement_str.size()), &statement, nullptr
    );
    if (ret_code != SQLITE_OK) {
        throw SQLiteException(
            sqlite3_errmsg(db_handler_.GetHandle()), ret_code, sqlite3_extended_errcode(db_handler_.GetHandle())
        );
    }

    return Statement::NativeStatementPtr(statement, SQLiteStatementDeleter());
}

void Statement::CheckCode(const int ret_code) const {
    if (ret_code != SQLITE_OK) {
        throw SQLiteException(
            sqlite3_errmsg(db_handler_.GetHandle()), ret_code, sqlite3_extended_errcode(db_handler_.GetHandle())
        );
    }
}

void Statement::Bind(const int index, const int32_t value) {
    const int ret_code = sqlite3_bind_int(prepare_statement_.get(), index, value);
    CheckCode(ret_code);
}

void Statement::Bind(const int index, const int64_t value) {
    const int ret_code = sqlite3_bind_int64(prepare_statement_.get(), index, value);
    CheckCode(ret_code);
}

void Statement::Bind(const int index, const uint32_t value) {
    const int ret_code = sqlite3_bind_int64(prepare_statement_.get(), index, value);
    CheckCode(ret_code);
}

void Statement::Bind(const int index, const uint64_t value) {
    const int ret_code = sqlite3_bind_int64(prepare_statement_.get(), index, value);
    CheckCode(ret_code);
}

void Statement::Bind(const int index, const double value) {
    const int ret_code = sqlite3_bind_double(prepare_statement_.get(), index, value);
    CheckCode(ret_code);
}

void Statement::Bind(const int index, const std::string& value) {
    const int ret_code = sqlite3_bind_text(
        prepare_statement_.get(), index, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT
    );
    CheckCode(ret_code);
}

void Statement::Bind(const int index, const std::string_view value) {
    const int ret_code = sqlite3_bind_text(
        prepare_statement_.get(), index, value.data(), static_cast<int>(value.size()), SQLITE_TRANSIENT
    );
    CheckCode(ret_code);
}

void Statement::Bind(const int index, const char* value, const int size) {
    const int ret_code = sqlite3_bind_blob(prepare_statement_.get(), index, value, size, SQLITE_TRANSIENT);
    CheckCode(ret_code);
}

void Statement::Bind(const int index, const std::vector<std::uint8_t>& value) {
    const int ret_code =
        sqlite3_bind_blob(prepare_statement_.get(), index, value.data(), value.size(), SQLITE_TRANSIENT);
    CheckCode(ret_code);
}

void Statement::Bind(const int index) {
    const int ret_code = sqlite3_bind_null(prepare_statement_.get(), index);
    CheckCode(ret_code);
}

std::int64_t Statement::RowsAffected() const noexcept {
    // TODO: on MacOS default out-of-the-box SQLite doesn't support sqlite3_changes64
    // extern "C" int sqlite3_changes64(sqlite3*);
    return sqlite3_changes64(sqlite3_db_handle(prepare_statement_.get()));
}

std::int64_t Statement::LastInsertRowId() const noexcept {
    return sqlite3_last_insert_rowid(sqlite3_db_handle(prepare_statement_.get()));
}

bool Statement::HasNext() const noexcept { return exec_status_ == SQLITE_ROW; }

bool Statement::IsDone() const noexcept { return exec_status_ == SQLITE_DONE; }

bool Statement::IsFail() const noexcept { return !IsDone() && !HasNext(); }

void Statement::Next() noexcept { exec_status_ = sqlite3_step(prepare_statement_.get()); }

void Statement::CheckStepStatus() {
    // If execution is finish reset statement and clear bindings
    if (!HasNext()) {
        Reset();
    }
    // Check if last exec_step finished with error
    if (IsFail()) {
        throw SQLiteException(
            sqlite3_errmsg(db_handler_.GetHandle()), exec_status_, sqlite3_extended_errcode(db_handler_.GetHandle())
        );
    }
}

int Statement::ColumnCount() const noexcept { return column_count_; }

void Statement::Reset() noexcept {
    // It's return last execution error status, we do not need to check it here
    sqlite3_reset(prepare_statement_.get());
    sqlite3_clear_bindings(prepare_statement_.get());  // reset all host parameters to NULL
}

bool Statement::IsNull(int column) const noexcept {
    return sqlite3_column_type(prepare_statement_.get(), column) == SQLITE_NULL;
}

void Statement::Extract(int column, int8_t& val) const noexcept {
    val = sqlite3_column_int(prepare_statement_.get(), column);
}

void Statement::Extract(int column, uint8_t& val) const noexcept {
    val = sqlite3_column_int(prepare_statement_.get(), column);
}

void Statement::Extract(int column, int16_t& val) const noexcept {
    val = sqlite3_column_int(prepare_statement_.get(), column);
}

void Statement::Extract(int column, uint16_t& val) const noexcept {
    val = sqlite3_column_int(prepare_statement_.get(), column);
}

void Statement::Extract(int column, int32_t& val) const noexcept {
    val = sqlite3_column_int(prepare_statement_.get(), column);
}

void Statement::Extract(int column, uint32_t& val) const noexcept {
    val = sqlite3_column_int64(prepare_statement_.get(), column);
}

void Statement::Extract(int column, int64_t& val) const noexcept {
    val = sqlite3_column_int64(prepare_statement_.get(), column);
}

void Statement::Extract(int column, uint64_t& val) const noexcept {
    val = sqlite3_column_int64(prepare_statement_.get(), column);
}

void Statement::Extract(int column, float& val) const noexcept {
    val = sqlite3_column_double(prepare_statement_.get(), column);
}

void Statement::Extract(int column, double& val) const noexcept {
    val = sqlite3_column_double(prepare_statement_.get(), column);
}

void Statement::Extract(int column, std::string& val) const noexcept {
    auto data = static_cast<const char*>(sqlite3_column_blob(prepare_statement_.get(), column));
    val = data ? std::string(data, sqlite3_column_bytes(prepare_statement_.get(), column)) : std::string{};
}

void Statement::Extract(int column, std::vector<uint8_t>& val) const noexcept {
    const void* blob = sqlite3_column_blob(prepare_statement_.get(), column);
    val = blob ? std::vector<uint8_t>(
                     static_cast<const uint8_t*>(blob),
                     static_cast<const uint8_t*>(blob) + sqlite3_column_bytes(prepare_statement_.get(), column)
                 )
               : std::vector<uint8_t>{};
}

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
