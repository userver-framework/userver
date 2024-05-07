#include <userver/ydb/exceptions.hpp>

#include <ydb-cpp-sdk/client/types/status/status.h>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace ydb {

YdbResponseError::YdbResponseError(std::string_view operation_name,
                                   NYdb::TStatus&& status)
    : BaseError(fmt::format("YDB {} failed: {}", operation_name,
                            status.GetIssues().ToOneLineString())),
      status_(std::move(status)) {}

const NYdb::TStatus& YdbResponseError::GetStatus() const noexcept {
  return status_;
}

UndefinedDatabaseError::UndefinedDatabaseError(std::string error)
    : BaseError(std::move(error)) {}

EmptyResponseError::EmptyResponseError(std::string_view expected)
    : BaseError(fmt::format(
          "Unexpected YDB empty response: no response and no errors for '{}'",
          expected)) {}

ParseError::ParseError(std::string error) : BaseError(std::move(error)) {}

ColumnParseError::ColumnParseError(std::string_view column_name,
                                   std::string_view message)
    : ParseError(
          fmt::format("Unexpected contents of column '{}' in YDB response: {}",
                      column_name, message)) {}

TypeMismatchError::TypeMismatchError(std::string_view name,
                                     std::string_view expected,
                                     std::string_view actual)
    : BaseError(
          fmt::format("Field '{}' is of a wrong type. Expected: {}, actual: {}",
                      name, expected, actual)) {}

DeadlineExceededError::DeadlineExceededError(std::string msg)
    : BaseError(std::move(msg)) {}

OperationCancelledError::OperationCancelledError()
    : BaseError("Cannot wait for future: operation cancelled") {}

InvalidTransactionError::InvalidTransactionError()
    : BaseError("Transaction is used after it is already finished") {}

ResponseTruncatedError::ResponseTruncatedError()
    : BaseError("Response is truncated") {}

TransactionForceRollback::TransactionForceRollback()
    : BaseError("Force rollback due to Testpoint response") {}

IgnoreResultsError::IgnoreResultsError(std::string err) : BaseError(err) {}

}  // namespace ydb

USERVER_NAMESPACE_END
