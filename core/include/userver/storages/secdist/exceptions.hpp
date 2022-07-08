#pragma once

#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace storages::secdist {

class SecdistError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class InvalidSecdistJson : public SecdistError {
  using SecdistError::SecdistError;
};

class UnknownMongoDbAlias : public SecdistError {
  using SecdistError::SecdistError;
};

class UnknownRedisClientName : public SecdistError {
  using SecdistError::SecdistError;
};

class UnknownPostgresDbAlias : public SecdistError {
  using SecdistError::SecdistError;
};

}  // namespace storages::secdist

USERVER_NAMESPACE_END
