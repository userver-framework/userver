#pragma once

#include <optional>
#include <string>

#include <sql.h>
#include <sqlext.h>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

enum class TransactionCapability : SQLUSMALLINT {
    kNone = SQL_TC_NONE,
    kDml = SQL_TC_DML,
    kAll = SQL_TC_ALL,
    kDdlCommit = SQL_TC_DDL_COMMIT,
    kDdlIgnore = SQL_TC_DDL_IGNORE,
};

enum class ParameterArrayRowCounts : SQLUINTEGER {
    kBatch = SQL_PARC_BATCH,
    kNoBatch = SQL_PARC_NO_BATCH,
};

enum class ParameterArraySelects : SQLUINTEGER {
    kBatch = SQL_PAS_BATCH,
    kNoBatch = SQL_PAS_NO_BATCH,
    kNoSelect = SQL_PAS_NO_SELECT,
};

enum class CursorBehavior : SQLUSMALLINT {
    kDelete = SQL_CB_DELETE,
    kClose = SQL_CB_CLOSE,
    kPreserve = SQL_CB_PRESERVE,
};

/// Metadata reported by one physical ODBC connection immediately after connect.
///
/// The snapshot deliberately preserves raw ODBC masks and distinct enum values:
/// their interpretation belongs to the operation that will eventually gate on
/// them. A missing optional value means that SQLGetInfo itself was unsupported.
class DriverCapabilities final {
public:
    DriverCapabilities() = default;

    static DriverCapabilities Read(SQLHDBC connection, engine::Deadline deadline);

    const std::string& GetDbmsName() const noexcept { return dbms_name_; }
    const std::string& GetDbmsVersion() const noexcept { return dbms_version_; }
    const std::string& GetDriverName() const noexcept { return driver_name_; }
    const std::string& GetDriverVersion() const noexcept { return driver_version_; }
    const std::string& GetDriverOdbcVersion() const noexcept { return driver_odbc_version_; }

    std::optional<TransactionCapability> GetTransactionCapability() const noexcept { return transaction_capability_; }
    std::optional<SQLUINTEGER> GetTransactionIsolationOptions() const noexcept {
        return transaction_isolation_options_;
    }
    std::optional<SQLUINTEGER> GetDefaultTransactionIsolation() const noexcept {
        return default_transaction_isolation_;
    }
    std::optional<bool> IsDataSourceReadOnly() const noexcept { return data_source_read_only_; }
    std::optional<bool> CanDescribeParameters() const noexcept { return describe_parameter_; }

    std::optional<ParameterArrayRowCounts> GetParameterArrayRowCounts() const noexcept {
        return parameter_array_row_counts_;
    }
    std::optional<ParameterArraySelects> GetParameterArraySelects() const noexcept { return parameter_array_selects_; }
    std::optional<SQLUINTEGER> GetBatchRowCount() const noexcept { return batch_row_count_; }

    std::optional<SQLUINTEGER> GetScrollOptions() const noexcept { return scroll_options_; }
    std::optional<SQLUINTEGER> GetGetDataExtensions() const noexcept { return getdata_extensions_; }
    std::optional<CursorBehavior> GetCursorCommitBehavior() const noexcept { return cursor_commit_behavior_; }
    std::optional<CursorBehavior> GetCursorRollbackBehavior() const noexcept { return cursor_rollback_behavior_; }

private:
    std::string dbms_name_;
    std::string dbms_version_;
    std::string driver_name_;
    std::string driver_version_;
    std::string driver_odbc_version_;

    std::optional<TransactionCapability> transaction_capability_;
    std::optional<SQLUINTEGER> transaction_isolation_options_;
    std::optional<SQLUINTEGER> default_transaction_isolation_;
    std::optional<bool> data_source_read_only_;
    std::optional<bool> describe_parameter_;

    std::optional<ParameterArrayRowCounts> parameter_array_row_counts_;
    std::optional<ParameterArraySelects> parameter_array_selects_;
    std::optional<SQLUINTEGER> batch_row_count_;

    std::optional<SQLUINTEGER> scroll_options_;
    std::optional<SQLUINTEGER> getdata_extensions_;
    std::optional<CursorBehavior> cursor_commit_behavior_;
    std::optional<CursorBehavior> cursor_rollback_behavior_;
};

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
