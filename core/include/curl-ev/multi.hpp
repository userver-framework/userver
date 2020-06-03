/** @file curl-ev/multi.hpp
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        Integration of libcurl's multi interface with Boost.Asio
*/

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <set>

#include <utils/token_bucket.hpp>
#include "error_code.hpp"
#include "initialization.hpp"
#include "multi_statistics.hpp"
#include "native.hpp"

namespace engine {
namespace ev {

class ThreadControl;
class AsyncWatcher;
class TimerWatcher;

}  // namespace ev
}  // namespace engine

namespace curl {
class easy;
struct socket_info;

class multi final {
 public:
  using Callback = std::function<void()>;
  multi(engine::ev::ThreadControl& thread_control);
  multi(const multi&) = delete;
  ~multi();

  inline native::CURLM* native_handle() { return handle_; }

  void add(easy* easy_handle);
  void remove(easy* easy_handle);

  void BindEasySocket(easy&, native::curl_socket_t);
  void UnbindEasySocket(native::curl_socket_t);

  engine::ev::ThreadControl& GetThreadControl() { return thread_control_; }

  MultiStatistics& Statistics() { return statistics_; }

  void SetConnectRatelimitHttp(
      size_t max_size, utils::TokenBucket::Duration token_update_interval);

  void SetConnectRatelimitHttps(
      size_t max_size, utils::TokenBucket::Duration token_update_interval);

  bool MayAcquireConnectionHttp(const std::string& url);

  bool MayAcquireConnectionHttps(const std::string& url);

  void SetMultiplexingEnabled(bool);
  void SetMaxHostConnections(long);
  void SetConnectionCacheSize(long);

 private:
  void add_handle(native::CURL* native_easy);
  void remove_handle(native::CURL* native_easy);

  void assign(native::curl_socket_t sockfd, void* user_data);
  void socket_action(native::curl_socket_t s, int event_bitmask);

  using socket_function_t = int (*)(native::CURL* native_easy,
                                    native::curl_socket_t s, int what,
                                    void* userp, void* socketp);
  void set_socket_function(socket_function_t socket_function);
  void set_socket_data(void* socket_data);

  using timer_function_t = int (*)(native::CURLM* native_multi, long timeout_ms,
                                   void* userp);
  void set_timer_function(timer_function_t timer_function);
  void set_timer_data(void* timer_data);

  void SetOptionAsync(native::CURLMoption, long);

  void monitor_socket(socket_info* si, int action);
  void process_messages();
  bool still_running();

  void start_read_op(socket_info* si);
  void handle_socket_read(std::error_code err, socket_info* si);
  void start_write_op(socket_info* si);
  void handle_socket_write(std::error_code err, socket_info* si);
  void handle_timeout(const std::error_code& err);
  void handle_async();

  socket_info* GetSocketInfo(native::curl_socket_t);

  static int socket(native::CURL* native_easy, native::curl_socket_t s,
                    int what, void* userp, void* socketp) noexcept;
  static int timer(native::CURLM* native_multi, long timeout_ms,
                   void* userp) noexcept;

  class Impl;
  std::unique_ptr<Impl> pimpl_;
  native::CURLM* handle_;
  engine::ev::ThreadControl& thread_control_;
  MultiStatistics statistics_;
  utils::TokenBucket connect_ratelimit_http_;
  utils::TokenBucket connect_ratelimit_https_;
};
}  // namespace curl
