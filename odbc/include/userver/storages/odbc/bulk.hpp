#pragma once

/// @file userver/storages/odbc/bulk.hpp
/// @brief Owning parameters and execution outcome for ODBC bulk DML.

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

#include <userver/storages/odbc/exception.hpp>
#include <userver/storages/odbc/parameter_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

inline constexpr std::size_t kDefaultBulkRows = 1000;

/// Status of one input row in an ODBC bulk execution.
enum class BulkRowStatus {
    /// Driver confirmed successful execution without diagnostics.
    kSuccess,
    /// Driver confirmed success and reported warning-class diagnostics.
    kSuccessWithInfo,
    /// Driver reported that this row failed.
    kError,
    /// Driver reported that this row was not used.
    kUnused,
    /// The row was processed, but row-specific diagnostics are unavailable.
    kDiagnosticsUnavailable,
    /// The driver did not provide a trustworthy per-row status.
    kUnknown,
};

/// @brief Owning, ordered rows of parameters for ODBC bulk DML.
///
/// The first row fixes the column count and every later row must match it.
/// Columns must also have one normalized type in every row. Use an empty
/// `std::optional<T>` for SQL NULL; raw `nullptr` and `std::nullopt` are
/// untyped and are rejected by bulk preflight.
class BulkParameterStore final {
public:
    BulkParameterStore() = default;
    BulkParameterStore(const BulkParameterStore&) = delete;
    BulkParameterStore(BulkParameterStore&&) noexcept = default;
    BulkParameterStore& operator=(const BulkParameterStore&) = delete;
    BulkParameterStore& operator=(BulkParameterStore&&) noexcept = default;

    /// Append a row copied from values accepted by ParameterStore.
    template <typename... Args>
    requires((impl::kIsParameterArgument<Args> && ...))
    BulkParameterStore& PushBackRow(const Args&... args) {
        AppendRow(impl::MakeParameterList(args...));
        return *this;
    }

    /// Append a row, transferring its owned parameter values.
    BulkParameterStore& PushBackRow(ParameterStore&& row);

    bool IsEmpty() const noexcept { return rows_.empty(); }
    std::size_t RowsCount() const noexcept { return rows_.size(); }
    std::size_t ColumnsCount() const noexcept { return columns_count_; }

private:
    friend class Cluster;
    friend class Transaction;

    void AppendRow(impl::ParameterList row);
    const impl::ParameterRows& GetRows() const noexcept { return rows_; }

    impl::ParameterRows rows_;
    std::size_t columns_count_{0};
};

/// @brief Outcome snapshot of an ODBC bulk execution.
///
/// `Processed()` is absent when the driver did not provide a reliable count.
/// `RowsAffected()` is absent when any DML result reported an unknown count.
/// `Succeeded()` counts only kSuccess and kSuccessWithInfo rows; unknown and
/// diagnostics-unavailable rows are deliberately not assumed successful.
class BulkResult final {
public:
    BulkResult() = default;
    BulkResult(
        std::size_t requested,
        std::optional<std::size_t> processed,
        std::optional<std::size_t> rows_affected,
        std::vector<BulkRowStatus> statuses
    );

    /// Number of input rows. Always equals `Statuses().size()`.
    std::size_t Requested() const noexcept { return requested_; }
    /// Reliable processed-row count, if supplied by the driver.
    std::optional<std::size_t> Processed() const noexcept { return processed_; }
    /// Number of rows with a confirmed successful status.
    std::size_t Succeeded() const noexcept { return succeeded_; }
    /// Checked aggregate DML row count, or null when any count is unknown.
    std::optional<std::size_t> RowsAffected() const noexcept { return rows_affected_; }
    /// One status for every requested row, including unused tail rows.
    const std::vector<BulkRowStatus>& Statuses() const noexcept { return statuses_; }

private:
    std::size_t requested_{0};
    std::optional<std::size_t> processed_{0};
    std::size_t succeeded_{0};
    std::optional<std::size_t> rows_affected_{0};
    std::vector<BulkRowStatus> statuses_;
};

/// Bulk DML failed after possibly executing a subset of the requested rows.
///
/// The result snapshot is observational: execution is never retried after
/// `SQLExecute` starts. In direct autocommit mode, completed chunks or scalar
/// fallback rows may already be committed. Use `Transaction::ExecuteBulk` and
/// roll back on failure when atomicity is required.
class BulkExecutionError : public StatementError {
public:
    BulkExecutionError(
        std::string message,
        std::vector<DiagnosticRecord> diagnostics,
        BulkResult result,
        bool invalid_handle = false
    );

    const BulkResult& GetResult() const noexcept { return result_; }

private:
    BulkResult result_;
};

}  // namespace storages::odbc

USERVER_NAMESPACE_END
