#pragma once

/// @file storages/mongo_ng/exception.hpp
/// @brief MongoDB-specific exceptions

#include <stdexcept>

namespace storages::mongo_ng {

/// Generic mongo-related exception
class MongoException : public std::exception {
 public:
  explicit MongoException(std::string msg) : msg_(std::move(msg)) {}
  virtual ~MongoException() = default;

  virtual const char* what() const noexcept override { return msg_.c_str(); }

 private:
  std::string msg_;
};

/// Client pool error
class PoolException : public MongoException {
 public:
  PoolException(std::string pool_id, std::string msg)
      : MongoException(msg + " in pool '" + pool_id + '\''),
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
