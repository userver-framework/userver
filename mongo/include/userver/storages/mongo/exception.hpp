#pragma once

/// @file userver/storages/mongo/exception.hpp
/// @brief MongoDB-specific exceptions

#include <userver/utils/traceful_exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo {

/// Generic mongo-related exception
class MongoException : public utils::TracefulException {
 public:
  MongoException();

  explicit MongoException(std::string_view what);
};

/// Config validation error
class InvalidConfigException : public MongoException {
  using MongoException::MongoException;
};

/// The current task has been cancelled, e.g. by deadline propagation
class CancelledException : public MongoException {
 public:
  using MongoException::MongoException;
};

/// Nonexistent pool requested from the set
class PoolNotFoundException : public MongoException {
  using MongoException::MongoException;
};

/// Pool refused to satisfy connection request due to high load
class PoolOverloadException : public MongoException {
  using MongoException::MongoException;
};

/// Network (connectivity) error
class NetworkException : public MongoException {
 public:
  using MongoException::MongoException;
};

/// No server available to satisfy request constraints
class ClusterUnavailableException : public MongoException {
 public:
  using MongoException::MongoException;
};

/// Incompatible server version
class IncompatibleServerException : public MongoException {
 public:
  using MongoException::MongoException;
};

/// Authentication error
class AuthenticationException : public MongoException {
 public:
  using MongoException::MongoException;
};

/// Generic query error
class QueryException : public MongoException {
 public:
  using MongoException::MongoException;
};

/// Query argument validation error
class InvalidQueryArgumentException : public QueryException {
 public:
  using QueryException::QueryException;
};

/// Server-side error
class ServerException : public QueryException {
 public:
  explicit ServerException(int code) : code_(code) {}

  int Code() const { return code_; }

 private:
  int code_;
};

/// Write concern error
class WriteConcernException : public ServerException {
 public:
  using ServerException::ServerException;
};

/// Duplicate key error
class DuplicateKeyException : public ServerException {
 public:
  using ServerException::ServerException;
};

}  // namespace storages::mongo

USERVER_NAMESPACE_END
