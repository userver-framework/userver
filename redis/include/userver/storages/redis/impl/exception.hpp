#pragma once

/// @file userver/storages/redis/impl/exception.hpp
/// @brief redis-specific exceptions

#include <stdexcept>
#include <string_view>

#include <userver/storages/redis/impl/reply_status.hpp>

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

/// Request execution failed
class RequestFailedException : public Exception {
 public:
  RequestFailedException(const std::string& request_description,
                         ReplyStatus status);

  ReplyStatus GetStatus() const;
  std::string_view GetStatusString() const;

  bool IsTimeout() const;

 private:
  ReplyStatus status_;
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
