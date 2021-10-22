#pragma once

/// @file redis/exception.hpp
/// @brief redis-specific exceptions

#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace redis {

/// Generic redis-related exception
class Exception : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/// Invalid redis command argument
class InvalidArgumentException : public Exception {
 public:
  using Exception::Exception;
};

/// No reply from redis server
class RequestFailedException : public Exception {
 public:
  RequestFailedException(const std::string& request_description, int status);

  int GetStatus() const;
  const std::string& GetStatusString() const;

  bool IsTimeout() const;

 private:
  int status_;
};

/// Request was cancelled
class RequestCancelledException : public Exception {
 public:
  using Exception::Exception;
};

/// Invalid reply data format
class ParseReplyException : public Exception {
 public:
  using Exception::Exception;
};

/// Invalid config format
class ParseConfigException : public Exception {
 public:
  using Exception::Exception;
};

/// Cannot connect to some redis server shard
class ClientNotConnectedException : public Exception {
 public:
  using Exception::Exception;
};

}  // namespace redis

USERVER_NAMESPACE_END
