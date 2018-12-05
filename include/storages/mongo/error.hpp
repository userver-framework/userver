#pragma once

#include <storages/mongo/mongo.hpp>

namespace storages {
namespace mongo {

/// Base class for mongo-related errors
class MongoError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

/// Client pool error
class PoolError : public MongoError {
  using MongoError::MongoError;
};

/// Config validation error
class InvalidConfig : public PoolError {
  using PoolError::PoolError;
};

class PoolNotFound : public PoolError {
  using PoolError::PoolError;
};

/// Type conversion failure
class BadType : public MongoError {
 public:
  BadType(const DocumentElement& item, const char* context);
  BadType(const ArrayElement& item, const char* context);
};

}  // namespace mongo
}  // namespace storages
