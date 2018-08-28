#pragma once

#include <exception>
#include <string>
#include <system_error>

namespace clients {
namespace http {

/// Exception with string
class BaseException : public std::exception {
 public:
  BaseException() = default;
  explicit BaseException(std::string msg) : msg_(std::move(msg)) {}
  virtual ~BaseException() = default;

  virtual const char* what() const noexcept override { return msg_.c_str(); }

 protected:
  std::string msg_;
};

/// Exception with string and error_code
class BaseCodeException : public BaseException {
 public:
  BaseCodeException(std::error_code ec, const std::string& msg);
  virtual ~BaseCodeException() = default;

  const std::error_code& error_code() const noexcept { return ec_; }

 protected:
  std::error_code ec_;
};

class TimeoutException : public BaseException {
 public:
  using BaseException::BaseException;
  virtual ~TimeoutException() = default;
};

class SSLException : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  virtual ~SSLException() = default;
};

class TechnicalError : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  virtual ~TechnicalError() = default;
};

class BadArgumentException : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  virtual ~BadArgumentException() = default;
};

class TooManyRedirectsException : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  virtual ~TooManyRedirectsException() = default;
};

class NetworkProblemException : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  virtual ~NetworkProblemException() = default;
};

class DNSProblemException : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  virtual ~DNSProblemException() = default;
};

class AuthFailedException : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  virtual ~AuthFailedException() = default;
};

/// Base class for HttpClientException and HttpServerException
class HttpException : public BaseException {
 public:
  HttpException(long code) : code_(code) {
    msg_ = "Raise for status exception, code = " + std::to_string(code);
  }
  virtual ~HttpException() = default;

  long code() const { return code_; }

 protected:
  long code_;
};

class HttpClientException : public HttpException {
 public:
  using HttpException::HttpException;
  virtual ~HttpClientException() = default;
};

class HttpServerException : public HttpException {
 public:
  using HttpException::HttpException;
  virtual ~HttpServerException() = default;
};

/// map error_code to exceptions
std::exception_ptr PrepareException(std::error_code ec);

}  // namespace http
}  // namespace clients
