#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql::impl {

class MySQLConnection;

class BrokenGuard final {
 public:
  explicit BrokenGuard(MySQLConnection& connection);
  ~BrokenGuard();

  void ThrowGeneralException(unsigned int error, std::string message);

  void ThrowStatementException(unsigned int error, std::string message);

  void ThrowCommandException(unsigned int error, std::string message);

  void ThrowTransactionException(unsigned int error, std::string message);

 private:
  MySQLConnection& connection_;

  int exceptions_on_enter_;
  unsigned int errno_{0};
};

}  // namespace storages::mysql::impl

USERVER_NAMESPACE_END
