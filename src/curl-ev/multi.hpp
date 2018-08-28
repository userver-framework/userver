/**
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

#include <engine/ev/watcher/async_watcher.hpp>
#include <engine/ev/watcher/timer_watcher.hpp>
#include "config.hpp"
#include "error_code.hpp"
#include "initialization.hpp"
#include "native.hpp"
#include "socket_info.hpp"

#include <engine/ev/thread.hpp>
#include <engine/task/task_processor.hpp>

#define IMPLEMENT_CURL_MOPTION_BOOLEAN(FUNCTION_NAME, OPTION_NAME)           \
  inline void FUNCTION_NAME(bool enabled) {                                  \
    std::error_code ec;                                                      \
    FUNCTION_NAME(enabled, ec);                                              \
    throw_error(ec, PP_STRINGIZE(FUNCTION_NAME));                            \
  }                                                                          \
  inline void FUNCTION_NAME(bool enabled, std::error_code& ec) {             \
    ec = std::error_code(                                                    \
        native::curl_multi_setopt(handle_, OPTION_NAME, enabled ? 1L : 0L)); \
  }

#define IMPLEMENT_CURL_MOPTION(FUNCTION_NAME, OPTION_NAME, OPTION_TYPE)        \
  inline void FUNCTION_NAME(OPTION_TYPE arg) {                                 \
    std::error_code ec;                                                        \
    FUNCTION_NAME(arg, ec);                                                    \
    throw_error(ec, PP_STRINGIZE(FUNCTION_NAME));                              \
  }                                                                            \
  inline void FUNCTION_NAME(OPTION_TYPE arg, std::error_code& ec) {            \
    ec =                                                                       \
        std::error_code(native::curl_multi_setopt(handle_, OPTION_NAME, arg)); \
  }

#define IMPLEMENT_CURL_MOPTION_ENUM(FUNCTION_NAME, OPTION_NAME, ENUM_TYPE, \
                                    OPTION_TYPE)                           \
  inline void FUNCTION_NAME(ENUM_TYPE arg) {                               \
    std::error_code ec;                                                    \
    FUNCTION_NAME(arg, ec);                                                \
    throw_error(ec, PP_STRINGIZE(FUNCTION_NAME));                          \
  }                                                                        \
  inline void FUNCTION_NAME(ENUM_TYPE arg, std::error_code& ec) {          \
    ec = std::error_code(native::curl_multi_setopt(                        \
        handle_, OPTION_NAME, static_cast<OPTION_TYPE>(arg)));             \
  }

namespace curl {
class easy;

class CURLASIO_API multi {
 public:
  using Callback = std::function<void()>;
  multi(engine::ev::ThreadControl& thread_control);
  multi(const multi&) = delete;
  ~multi();

  inline native::CURLM* native_handle() { return handle_; }

  void add(easy* easy_handle);
  void remove(easy* easy_handle);

  void socket_register(std::shared_ptr<socket_info> si);
  void socket_cleanup(native::curl_socket_t s);

  bool empty() const { return easy_handles_.empty() && still_running_ == 0; }

  engine::ev::ThreadControl& GetThreadControl() { return thread_control_; }

  enum pipelining_mode_t { pipe_nothing, pipe_http1, pipe_multiplex };
  IMPLEMENT_CURL_MOPTION_ENUM(set_pipelining, native::CURLMOPT_PIPELINING,
                              pipelining_mode_t, long);
  IMPLEMENT_CURL_MOPTION(set_max_pipeline_length,
                         native::CURLMOPT_MAX_PIPELINE_LENGTH, size_t);
  IMPLEMENT_CURL_MOPTION(set_max_host_connections,
                         native::CURLMOPT_MAX_HOST_CONNECTIONS, size_t);
  IMPLEMENT_CURL_MOPTION(set_max_connections, native::CURLMOPT_MAXCONNECTS,
                         long);

 private:
  typedef std::shared_ptr<socket_info> socket_info_ptr;

  void add_handle(native::CURL* native_easy);
  void remove_handle(native::CURL* native_easy);

  void assign(native::curl_socket_t sockfd, void* user_data);
  void socket_action(native::curl_socket_t s, int event_bitmask);
  socket_info_ptr register_cares_socket(native::curl_socket_t s);

  typedef int (*socket_function_t)(native::CURL* native_easy,
                                   native::curl_socket_t s, int what,
                                   void* userp, void* socketp);
  void set_socket_function(socket_function_t socket_function);
  void set_socket_data(void* socket_data);

  typedef int (*timer_function_t)(native::CURLM* native_multi, long timeout_ms,
                                  void* userp);
  void set_timer_function(timer_function_t timer_function);
  void set_timer_data(void* timer_data);

  void monitor_socket(socket_info_ptr si, int action);
  void process_messages();
  bool still_running();

  void start_read_op(socket_info_ptr si);
  void handle_socket_read(std::error_code err, socket_info_ptr si);
  void start_write_op(socket_info_ptr si);
  void handle_socket_write(std::error_code ec, socket_info_ptr si);
  void handle_timeout(const std::error_code& err);
  void handle_async();

  typedef EvSocket socket_type;
  typedef std::map<socket_type::native_handle_type, socket_info_ptr>
      socket_map_type;
  socket_map_type sockets_;
  socket_info_ptr get_socket_from_native(native::curl_socket_t native_socket);

  static int socket(native::CURL* native_easy, native::curl_socket_t s,
                    int what, void* userp, void* socketp);
  static int timer(native::CURLM* native_multi, long timeout_ms, void* userp);

  typedef std::set<easy*> easy_set_type;

  engine::ev::ThreadControl& thread_control_;
  initialization::ptr initref_;
  native::CURLM* handle_;
  engine::ev::AsyncWatcher timer_zero_watcher_;

  easy_set_type easy_handles_;
  engine::ev::TimerWatcher timer_;
  int still_running_;
};
}  // namespace curl
