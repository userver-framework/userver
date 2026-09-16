#include <userver/storages/odbc/bulk.hpp>

#include <algorithm>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc {

BulkParameterStore& BulkParameterStore::PushBackRow(ParameterStore&& row) {
    AppendRow(std::move(row.parameters_));
    return *this;
}

void BulkParameterStore::AppendRow(impl::ParameterList row) {
    if (row.empty()) {
        throw LogicError("ODBC bulk parameter rows must contain at least one column");
    }
    if (!rows_.empty() && row.size() != columns_count_) {
        throw LogicError(fmt::format("ODBC bulk parameter row has {} columns, expected {}", row.size(), columns_count_)
        );
    }
    if (rows_.empty()) {
        columns_count_ = row.size();
    }
    rows_.push_back(std::move(row));
}

BulkResult::BulkResult(
    std::size_t requested,
    std::optional<std::size_t> processed,
    std::optional<std::size_t> rows_affected,
    std::vector<BulkRowStatus> statuses
)
    : requested_{requested},
      processed_{processed},
      rows_affected_{rows_affected},
      statuses_{std::move(statuses)}
{
    if (statuses_.size() != requested_) {
        throw LogicError(
            fmt::format("ODBC bulk result contains {} statuses for {} requested rows", statuses_.size(), requested_)
        );
    }
    if (processed_ && *processed_ > requested_) {
        throw LogicError(
            fmt::format("ODBC bulk result reports {} processed rows for {} requested rows", *processed_, requested_)
        );
    }
    succeeded_ = static_cast<std::size_t>(std::count_if(statuses_.begin(), statuses_.end(), [](BulkRowStatus status) {
        return status == BulkRowStatus::kSuccess || status == BulkRowStatus::kSuccessWithInfo;
    }));
}

BulkExecutionError::BulkExecutionError(
    std::string message,
    std::vector<DiagnosticRecord> diagnostics,
    BulkResult result,
    bool invalid_handle
)
    : StatementError{std::move(message), std::move(diagnostics), invalid_handle},
      result_{std::move(result)}
{}

}  // namespace storages::odbc

USERVER_NAMESPACE_END
