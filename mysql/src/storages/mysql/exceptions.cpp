#include <userver/storages/mysql/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

MySQLException::MySQLException(unsigned int error, const char* message)
    : std::runtime_error{message}, errno_{error} {}

MySQLException::MySQLException(unsigned int error, const std::string& message)
    : std::runtime_error{message}, errno_{error} {}

MySQLException::~MySQLException() = default;

unsigned int MySQLException::GetErrno() const { return errno_; }

MySQLIOException::~MySQLIOException() = default;

MySQLStatementException::~MySQLStatementException() = default;

MySQLCommandException::~MySQLCommandException() = default;

MySQLTransactionException::~MySQLTransactionException() = default;

MySQLValidationException::~MySQLValidationException() = default;

}  // namespace storages::mysql

USERVER_NAMESPACE_END
