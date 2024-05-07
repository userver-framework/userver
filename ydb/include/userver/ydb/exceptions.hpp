#pragma once

#include <ydb-cpp-sdk/client/types/status/status.h>

#include <stdexcept>
#include <string>
#include <string_view>

namespace NYdb {
class TStatus;
}  // namespace NYdb

USERVER_NAMESPACE_BEGIN

namespace ydb {

class BaseError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class YdbResponseError : public BaseError {
 public:
  explicit YdbResponseError(std::string_view operation_name,
                            NYdb::TStatus&& status);

  const NYdb::TStatus& GetStatus() const noexcept;

 private:
  NYdb::TStatus status_;
};

class UndefinedDatabaseError : public BaseError {
 public:
  explicit UndefinedDatabaseError(std::string error);
};

class EmptyResponseError : public BaseError {
 public:
  explicit EmptyResponseError(std::string_view expected);
};

class ParseError : public BaseError {
 public:
  explicit ParseError(std::string error);
};

class ColumnParseError : public ParseError {
 public:
  ColumnParseError(std::string_view column_name, std::string_view message);
};

class TypeMismatchError : public BaseError {
 public:
  TypeMismatchError(std::string_view name, std::string_view expected,
                    std::string_view actual);
};

class DeadlineExceededError : public BaseError {
 public:
  explicit DeadlineExceededError(std::string msg);
};

class InvalidTransactionError : public BaseError {
 public:
  InvalidTransactionError();
};

class OperationCancelledError : public BaseError {
 public:
  OperationCancelledError();
};

class ResponseTruncatedError : public BaseError {
 public:
  ResponseTruncatedError();
};

class TransactionForceRollback : public BaseError {
 public:
  TransactionForceRollback();
};

class IgnoreResultsError : public BaseError {
 public:
  IgnoreResultsError(std::string err);
};

}  // namespace ydb

USERVER_NAMESPACE_END
