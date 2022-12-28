#include <storages/mysql/impl/broken_guard.hpp>

#include <storages/mysql/impl/mariadb_include.hpp>

#include <userver/storages/mysql/exceptions.hpp>

#include <storages/mysql/impl/mysql_connection.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

BrokenGuard::BrokenGuard(MySQLConnection& connection)
    : connection_{connection},
      exceptions_on_enter_{std::uncaught_exceptions()} {
  if (connection_.IsBroken()) {
    ThrowGeneralException(0, "Connection is broken.");
  }
}

BrokenGuard::~BrokenGuard() {
  if (exceptions_on_enter_ != std::uncaught_exceptions()) {
    // some exception is being thrown, but not via our interface.
    // Break the connection.
    if (errno_ == 0) {
      connection_.NotifyBroken();
    }

    const auto within_range = [this](unsigned int left, unsigned int right) {
      return left <= errno_ && errno_ <= right;
    };

    if (within_range(ER_ERROR_FIRST, ER_ERROR_LAST_SECTION_1) ||
        within_range(ER_ERROR_FIRST_SECTION_2, ER_ERROR_LAST_SECTION_2) ||
        within_range(ER_ERROR_FIRST_SECTION_3, ER_ERROR_LAST_SECTION_3) ||
        within_range(ER_ERROR_FIRST_SECTION_4, ER_ERROR_LAST_SECTION_4) ||
        within_range(ER_ERROR_FIRST_SECTION_5, ER_ERROR_LAST)) {
      // all of these are mysqld errors
      return;
    }

    if (within_range(CR_MIN_ERROR, CR_MAX_ERROR) ||
        within_range(CER_MIN_ERROR, CER_MAX_ERROR)) {
      // these are client errors, we are screwed
      connection_.NotifyBroken();
      return;
    }

    // Us being here means we are dealing with something peculiar.
    // Break the connection.
    connection_.NotifyBroken();
  }
}

void BrokenGuard::ThrowGeneralException(unsigned int error,
                                        std::string message) {
  errno_ = error;
  throw MySQLException{error, message};
}

void BrokenGuard::ThrowStatementException(unsigned int error,
                                          std::string message) {
  errno_ = error;
  throw MySQLStatementException{error, message};
}

void BrokenGuard::ThrowCommandException(unsigned int error,
                                        std::string message) {
  errno_ = error;
  throw MySQLCommandException{error, message};
}

void BrokenGuard::ThrowTransactionException(unsigned int error,
                                            std::string message) {
  errno_ = error;
  throw MySQLTransactionException{error, message};
}

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
