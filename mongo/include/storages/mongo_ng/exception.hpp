#pragma once

/// @file storages/mongo_ng/exception.hpp
/// @brief MongoDB-specific exceptions

#include <utils/traceful_exception.hpp>

namespace storages::mongo_ng {

/// Generic mongo-related exception
class MongoException : public utils::TracefulException {
 public:
  using utils::TracefulException::TracefulException;
};

/// Client pool error
class PoolException : public MongoException {
 public:
  explicit PoolException(std::string pool_id)
      : MongoException("in pool '" + pool_id + "': "),
        pool_id_(std::move(pool_id)) {}

  const std::string& PoolId() const { return pool_id_; }

 private:
  std::string pool_id_;
};

/// Config validation error
class InvalidConfigException : public PoolException {
  using PoolException::PoolException;
};

}  // namespace storages::mongo_ng
