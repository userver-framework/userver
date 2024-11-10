#include <userver/storages/sqlite/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite {

SQLiteException::SQLiteException(unsigned int error, const char* message)
    : std::runtime_error{message}, errno_{error} {}

SQLiteException::SQLiteException(unsigned int error, const std::string& message)
    : std::runtime_error{message}, errno_{error} {}

SQLiteException::~SQLiteException() = default;

unsigned int SQLiteException::GetErrno() const { return errno_; }

SQLiteStatementException::~SQLiteStatementException() = default;

SQLiteTransactionException::~SQLiteTransactionException() = default;

}  // namespace storages::sqlite

USERVER_NAMESPACE_END
