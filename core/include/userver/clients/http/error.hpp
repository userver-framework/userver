#pragma once

#include <exception>
#include <string>
#include <string_view>
#include <system_error>

#include <userver/clients/http/local_stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

/// Exception with string
class BaseException : public std::exception {
 public:
  BaseException() = default;
  BaseException(std::string message, const LocalStats& stats)
      : message_(std::move(message)), stats_(stats) {}
  ~BaseException() override = default;

  const char* what() const noexcept override { return message_.c_str(); }

  const LocalStats& GetStats() const { return stats_; }

 private:
  std::string message_;
  LocalStats stats_;
};

/// Exception with string and error_code
class BaseCodeException : public BaseException {
 public:
  BaseCodeException(std::error_code ec, std::string_view message,
                    std::string_view url, const LocalStats& stats);
  ~BaseCodeException() override = default;

  const std::error_code& error_code() const noexcept { return ec_; }

 private:
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
  HttpException(int code, const LocalStats& stats);
  HttpException(int code, const LocalStats& stats, std::string_view message);
  ~HttpException() override = default;

  int code() const { return code_; }

 private:
  int code_;
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
std::exception_ptr PrepareException(std::error_code ec, std::string_view url,
                                    const LocalStats& stats);

}  // namespace clients::http

USERVER_NAMESPACE_END
