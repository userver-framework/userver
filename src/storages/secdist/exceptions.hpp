#pragma once

#include <stdexcept>

namespace storages {
namespace secdist {

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

}  // namespace secdist
}  // namespace storages
