#pragma once

#include <exception>
#include <string>
#include <system_error>

#include <curl-ev/local_stats.hpp>

namespace clients::http {

/// Exception with string
class BaseException : public std::exception {
 public:
  BaseException() = default;
  BaseException(std::string msg, const curl::LocalStats& stats)
      : msg_(std::move(msg)), stats_(stats) {}
  ~BaseException() override = default;

  const char* what() const noexcept override { return msg_.c_str(); }

  const curl::LocalStats& GetStats() const { return stats_; }

 protected:
  std::string msg_;
  curl::LocalStats stats_;
};

/// Exception with string and error_code
class BaseCodeException : public BaseException {
 public:
  BaseCodeException(std::error_code ec, const std::string& msg,
                    const curl::LocalStats& stats);
  ~BaseCodeException() override = default;

  const std::error_code& error_code() const noexcept { return ec_; }

 protected:
  std::error_code ec_;
};

class TimeoutException : public BaseException {
 public:
  using BaseException::BaseException;
  ~TimeoutException() override = default;
};

class CancelException : public BaseException {
 public:
  using BaseException::BaseException;
  ~CancelException() override = default;
};

class SSLException : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  ~SSLException() override = default;
};

class TechnicalError : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  ~TechnicalError() override = default;
};

class BadArgumentException : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  ~BadArgumentException() override = default;
};

class TooManyRedirectsException : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  ~TooManyRedirectsException() override = default;
};

class NetworkProblemException : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  ~NetworkProblemException() override = default;
};

class DNSProblemException : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  ~DNSProblemException() override = default;
};

class AuthFailedException : public BaseCodeException {
 public:
  using BaseCodeException::BaseCodeException;
  ~AuthFailedException() override = default;
};

/// Base class for HttpClientException and HttpServerException
class HttpException : public BaseException {
 public:
  HttpException(long code) : code_(code) {
    msg_ = "Raise for status exception, code = " + std::to_string(code);
  }
  ~HttpException() override = default;

  long code() const { return code_; }

 protected:
  long code_;
};

class HttpClientException : public HttpException {
 public:
  using HttpException::HttpException;
  ~HttpClientException() override = default;
};

class HttpServerException : public HttpException {
 public:
  using HttpException::HttpException;
  ~HttpServerException() override = default;
};

/// map error_code to exceptions
std::exception_ptr PrepareException(std::error_code ec, const std::string& url,
                                    const curl::LocalStats& stats);

}  // namespace clients::http
