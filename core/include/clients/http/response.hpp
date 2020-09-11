#pragma once

#include <sstream>
#include <string>
#include <unordered_map>

#include <curl-ev/local_stats.hpp>

#include <utils/str_icase.hpp>

#include "error.hpp"

namespace clients::http {

/// https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
enum Status : uint16_t {
  OK = 200,
  Created = 201,
  NoContent = 204,
  BadRequest = 400,
  NotFound = 404,
  Conflict = 409,
  InternalServerError = 500,
  BadGateway = 502,
  ServiceUnavailable = 503,
  GatewayTimeout = 504,
  InsufficientStorage = 507,
  BandwithLimitExceeded = 509,
  WebServerIsDown = 520,
  ConnectionTimedOut = 522,
  OriginIsUnreachable = 523,
  TimeoutOccured = 524,
  SslHandshakeFailed = 525,
  InvalidSslCertificate = 526,
};

std::ostream& operator<<(std::ostream& os, Status s);

/// Headers class
using Headers = std::unordered_map<std::string, std::string,
                                   utils::StrIcaseHash, utils::StrIcaseEqual>;

/// Class that will be returned for successful request
class Response final {
 public:
  Response() = default;

  /// response stream
  std::ostringstream& sink_stream() { return response_stream_; }

  /// body as string
  std::string body() const { return response_stream_.str(); }
  /// return referece to headers
  const Headers& headers() const { return headers_; }
  Headers& headers() { return headers_; }

  /// status_code
  Status status_code() const;
  /// check status code
  bool IsOk() const { return status_code() == Status::OK; }

  static void RaiseForStatus(long code);

  void raise_for_status() const;

  /// returns statistics on request execution like const of opened sockets,
  /// connect time...
  curl::LocalStats GetStats() const;

  void SetStats(const curl::LocalStats& stats) { stats_ = stats; }
  void SetStatusCode(Status status_code) { status_code_ = status_code; }

 private:
  Headers headers_;
  std::ostringstream response_stream_;
  Status status_code_;
  curl::LocalStats stats_;
};

}  // namespace clients::http
