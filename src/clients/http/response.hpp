#pragma once

#include <curl-ev/local_timings.hpp>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include "error.hpp"
#include "wrapper.hpp"

#include <engine/future.hpp>

namespace curl {
class easy;
}  // namespace curl

namespace clients {
namespace http {

/// https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
enum Status : uint16_t {
  OK = 200,
  Created = 201,
  BadRequest = 400,
  NotFound = 404,
  Conflict = 409,
  InternalServerError = 500
};

/// Headers class
typedef std::unordered_map<std::string, std::string> Headers;

/// Class that will be returned for successful request
class Response {
 public:
  Response(std::shared_ptr<EasyWrapper> easy) : easy_(easy) {}

  /// response stream
  inline std::ostringstream& sink_stream() { return response_stream_; }

  /// body as string
  inline std::string body() const { return response_stream_.str(); }
  /// return referece to headers
  inline Headers& headers() { return headers_; }
  /// status_code
  long status_code() const;
  /// check status code
  bool IsOk() const { return status_code() == Status::OK; }
  /// total request time
  double total_time() const;
  /// connect time
  double connect_time() const;

  double name_lookup_time() const;

  double appconnect_time() const;

  double pretransfer_time() const;

  double starttransfer_time() const;

  static void RaiseForStatus(long code);

  void raise_for_status() const;

  curl::easy& easy();
  const curl::easy& easy() const;

  curl::LocalTimings local_timings() const;

 private:
  Headers headers_;
  std::ostringstream response_stream_;
  std::shared_ptr<EasyWrapper> easy_;
};

class ResponseFuture {
 public:
  ResponseFuture(engine::Future<std::shared_ptr<Response>> future,
                 std::chrono::milliseconds total_timeout,
                 std::shared_ptr<EasyWrapper> easy)
      : future_(std::move(future)),
        deadline_(std::chrono::system_clock::now() + total_timeout),
        easy_(std::move(easy)) {}

  ResponseFuture(ResponseFuture&& future)
      : future_(std::move(future.future_)),
        deadline_(future.deadline_),
        easy_(std::move(future.easy_)) {}

  ResponseFuture(const ResponseFuture&) = delete;

  ~ResponseFuture() {
    if (future_.IsValid()) {
      // TODO: log it?
      easy_->Easy().cancel();
    }
  }

  void operator=(const ResponseFuture&) = delete;

  void Cancel() {
    easy_->Easy().cancel();
    Detach();
  }

  void Detach() { future_ = {}; }

  std::future_status Wait() const { return future_.WaitUntil(deadline_); }

  std::shared_ptr<Response> Get() {
    if (Wait() == std::future_status::ready) return future_.Get();
    throw TimeoutException("Future timeout");
  }

 private:
  engine::Future<std::shared_ptr<Response>> future_;
  const std::chrono::system_clock::time_point deadline_;
  const std::shared_ptr<EasyWrapper> easy_;
};

}  // namespace http
}  // namespace clients

namespace curl {
std::basic_ostream<char, std::char_traits<char>>& operator<<(
    std::ostream& stream, const curl::easy& ceasy);
}  // namespace curl

std::ostream& operator<<(std::ostream& stream,
                         const clients::http::Response& response);
