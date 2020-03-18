/**
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        Integration of libcurl's multi interface with Boost.Asio
*/

#include <cstring>
#include <system_error>

#include <curl-ev/easy.hpp>
#include <curl-ev/error_code.hpp>
#include <curl-ev/multi.hpp>
#include <curl-ev/socket_info.hpp>

#include <crypto/openssl_lock.hpp>
#include <engine/ev/thread.hpp>
#include <engine/ev/watcher/async_watcher.hpp>
#include <engine/ev/watcher/timer_watcher.hpp>
#include <engine/task/task_processor.hpp>

namespace curl {

using easy_set_type = std::set<easy*>;
using BusyMarker = ::utils::statistics::BusyMarker;

using socket_map_type =
    std::map<socket_type::native_handle_type, multi::socket_info_ptr>;

class multi::Impl final {
 public:
  Impl(engine::ev::ThreadControl& thread_control, multi& object);

  // typedef EvSocket socket_type;
  socket_map_type sockets_;
  socket_info_ptr get_socket_from_native(native::curl_socket_t native_socket);

  initialization::ptr initref_;
  engine::ev::AsyncWatcher timer_zero_watcher_;

  easy_set_type easy_handles_;
  engine::ev::TimerWatcher timer_;
  int still_running_;
};

multi::Impl::Impl(engine::ev::ThreadControl& thread_control, multi& object)
    : timer_zero_watcher_(thread_control,
                          std::bind(&multi::handle_async, &object)),
      timer_(thread_control),
      still_running_(0) {
  crypto::impl::OpensslLock::Init();

  initref_ = initialization::ensure_initialization();
}

multi::multi(engine::ev::ThreadControl& thread_control)
    : pimpl_(std::make_unique<Impl>(thread_control, *this)),
      thread_control_(thread_control),
      connect_ratelimit_http_(SIZE_MAX, std::chrono::nanoseconds(1)),
      connect_ratelimit_https_(SIZE_MAX, std::chrono::nanoseconds(1)) {
  LOG_TRACE() << "multi::multi";

  handle_ = native::curl_multi_init();
  if (!handle_) {
    throw std::bad_alloc();
  }

  set_socket_function(&multi::socket);
  set_socket_data(this);

  set_timer_function(&multi::timer);
  set_timer_data(this);

  pimpl_->timer_zero_watcher_.Start();
}

multi::~multi() {
  while (!pimpl_->easy_handles_.empty()) {
    auto it = pimpl_->easy_handles_.begin();
    easy* easy_handle = *it;
    easy_handle->cancel();
  }

  if (handle_) {
    native::curl_multi_cleanup(handle_);
    handle_ = nullptr;
  }
}

void multi::add(easy* easy_handle) {
  pimpl_->easy_handles_.insert(easy_handle);
  add_handle(easy_handle->native_handle());
}

void multi::remove(easy* easy_handle) {
  auto it = pimpl_->easy_handles_.find(easy_handle);

  if (it != pimpl_->easy_handles_.end()) {
    pimpl_->easy_handles_.erase(it);
    remove_handle(easy_handle->native_handle());
  }
}

void multi::socket_register(std::shared_ptr<socket_info> si) {
  socket_type::native_handle_type fd = si->socket->native_handle();
  pimpl_->sockets_.insert(socket_map_type::value_type(fd, std::move(si)));
}

void multi::socket_cleanup(native::curl_socket_t s) {
  auto it = pimpl_->sockets_.find(s);

  if (it != pimpl_->sockets_.end()) {
    socket_info_ptr p = it->second;
    monitor_socket(p, CURL_POLL_NONE);
    p->socket.reset();
    pimpl_->sockets_.erase(it);
  }
}

void multi::SetConnectRatelimitHttps(
    size_t max_size, utils::TokenBucket::Duration token_update_interval) {
  connect_ratelimit_https_.SetMaxSize(max_size);
  connect_ratelimit_https_.SetUpdateInterval(token_update_interval);
}

void multi::SetConnectRatelimitHttp(
    size_t max_size, utils::TokenBucket::Duration token_update_interval) {
  connect_ratelimit_http_.SetMaxSize(max_size);
  connect_ratelimit_http_.SetUpdateInterval(token_update_interval);
}

bool multi::MayAcquireConnectionHttp() {
  return connect_ratelimit_http_.Obtain();
}

bool multi::MayAcquireConnectionHttps() {
  return connect_ratelimit_https_.Obtain();
}

void multi::add_handle(native::CURL* native_easy) {
  std::error_code ec(native::curl_multi_add_handle(handle_, native_easy));
  throw_error(ec, "add_handle");
}

void multi::remove_handle(native::CURL* native_easy) {
  std::error_code ec(native::curl_multi_remove_handle(handle_, native_easy));
  throw_error(ec, "remove_handle");
}

void multi::assign(native::curl_socket_t sockfd, void* user_data) {
  std::error_code ec(native::curl_multi_assign(handle_, sockfd, user_data));
  throw_error(ec, "multi_assign");
}

void multi::socket_action(native::curl_socket_t s, int event_bitmask) {
  std::error_code ec(native::curl_multi_socket_action(handle_, s, event_bitmask,
                                                      &pimpl_->still_running_));
  throw_error(ec, "socket_action");

  if (!still_running()) {
    pimpl_->timer_.Cancel();
  }
}

void multi::set_socket_function(socket_function_t socket_function) {
  std::error_code ec(native::curl_multi_setopt(
      handle_, native::CURLMOPT_SOCKETFUNCTION, socket_function));
  throw_error(ec, "set_socket_function");
}

void multi::set_socket_data(void* socket_data) {
  std::error_code ec(native::curl_multi_setopt(
      handle_, native::CURLMOPT_SOCKETDATA, socket_data));
  throw_error(ec, "set_socket_data");
}

void multi::set_timer_function(timer_function_t timer_function) {
  std::error_code ec(native::curl_multi_setopt(
      handle_, native::CURLMOPT_TIMERFUNCTION, timer_function));
  throw_error(ec, "set_timer_function");
}

void multi::set_timer_data(void* timer_data) {
  std::error_code ec(native::curl_multi_setopt(
      handle_, native::CURLMOPT_TIMERDATA, timer_data));
  throw_error(ec, "set_timer_data");
}

void multi::monitor_socket(socket_info_ptr si, int action) {
  si->monitor_read = !!(action & CURL_POLL_IN);
  si->monitor_write = !!(action & CURL_POLL_OUT);

  if (!si->socket) {
    // If libcurl already requested destruction of the socket, then no further
    // action is required.
    return;
  }

  BusyMarker busy(Statistics().get_busy_storage());

  if (si->monitor_read && !si->pending_read_op) {
    start_read_op(si);
  }

  if (si->monitor_write && !si->pending_write_op) {
    start_write_op(si);
  }

// The CancelIoEx API is only available on Windows Vista and up. On Windows XP,
// the cancel operation therefore falls
// back to CancelIo(), which only works in single-threaded environments and
// depends on driver support, which is not always available.
// Therefore, this code section is only enabled when explicitly targeting
// Windows Vista and up or a non-Windows platform.
// On Windows XP and previous versions, the I/O handler will be executed and
// immediately return.
#if !defined(BOOST_WINDOWS_API) || \
    (defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0600))
  if (action == CURL_POLL_NONE &&
      (si->pending_read_op || si->pending_write_op)) {
    si->socket->cancel();
  }
#endif
}

void multi::process_messages() {
  native::CURLMsg* msg;
  int msgs_left;

  while ((msg = native::curl_multi_info_read(handle_, &msgs_left))) {
    if (msg->msg == native::CURLMSG_DONE) {
      easy* easy_handle = easy::from_native(msg->easy_handle);
      std::error_code ec;

      // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
      if (msg->data.result != native::CURLE_OK) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
        ec = std::error_code(msg->data.result);
      }

      remove(easy_handle);
      LOG_TRACE() << "mult::process_messages() handle_completion";
      easy_handle->handle_completion(ec);
    }
  }
}

bool multi::still_running() { return (pimpl_->still_running_ > 0); }

void multi::start_read_op(socket_info_ptr si) {
  LOG_TRACE() << "start_read_op si=" << reinterpret_cast<long>(si.get());

  si->pending_read_op = true;
  if (!si->is_cares_socket()) si->handle->timings().mark_start_read();

  si->socket->ReadAsync(
      std::bind(&multi::handle_socket_read, this, std::placeholders::_1, si));
}

void multi::handle_socket_read(std::error_code err, socket_info_ptr si) {
  LOG_TRACE() << "handle_socket_read si=" << reinterpret_cast<long>(si.get())
              << " ec=" << err
              << " socket=" << reinterpret_cast<long>(si->socket.get());
  if (!si->socket) {
    si->pending_read_op = false;
    return;
  }

  if (!err) {
    socket_action(si->orig_libcurl_socket, CURL_CSELECT_IN);
    process_messages();

    if (si->monitor_read)
      start_read_op(si);
    else
      si->pending_read_op = false;
  } else {
    if (err != std::make_error_code(std::errc::operation_canceled)) {
      socket_action(si->orig_libcurl_socket, CURL_CSELECT_ERR);
      process_messages();
    }

    si->pending_read_op = false;
  }
}

void multi::start_write_op(socket_info_ptr si) {
  LOG_TRACE() << "start_write_op si=" << reinterpret_cast<long>(si.get());
  if (!si->is_cares_socket()) si->handle->timings().mark_start_write();
  si->pending_write_op = true;

  si->socket->WriteAsync(
      std::bind(&multi::handle_socket_write, this, std::placeholders::_1, si));
}

void multi::handle_socket_write(std::error_code err, socket_info_ptr si) {
  LOG_TRACE() << "handle_socket_write si=" << reinterpret_cast<long>(si.get())
              << " ec=" << err
              << " socket=" << reinterpret_cast<long>(si->socket.get());
  if (!si->socket) {
    si->pending_write_op = false;
    return;
  }

  BusyMarker busy(Statistics().get_busy_storage());

  if (!err) {
    socket_action(si->orig_libcurl_socket, CURL_CSELECT_OUT);
    process_messages();

    if (si->monitor_write)
      start_write_op(si);
    else
      si->pending_write_op = false;
  } else {
    if (err != std::make_error_code(std::errc::operation_canceled)) {
      socket_action(si->orig_libcurl_socket, CURL_CSELECT_ERR);
      process_messages();
    }

    si->pending_write_op = false;
  }
}

void multi::handle_timeout(const std::error_code& err) {
  if (!err) {
    BusyMarker busy(Statistics().get_busy_storage());
    LOG_TRACE() << "handle_timeout " << reinterpret_cast<long long>(this);
    socket_action(CURL_SOCKET_TIMEOUT, 0);
    process_messages();
  }
}

multi::socket_info_ptr multi::get_socket_from_native(
    native::curl_socket_t native_socket) {
  auto it = pimpl_->sockets_.find(native_socket);

  if (it != pimpl_->sockets_.end()) {
    return it->second;
  } else {
    return socket_info_ptr();
  }
}

int multi::socket(native::CURL* native_easy, native::curl_socket_t s, int what,
                  void* userp, void* socketp) {
  auto* self = static_cast<multi*>(userp);
  LOG_TRACE() << "socket multi=" << reinterpret_cast<long>(self)
              << " what=" << what << " &si=" << reinterpret_cast<long>(socketp);

  if (what == CURL_POLL_REMOVE) {
    // stop listening for events
    if (socketp) {
      auto si = static_cast<socket_info_ptr*>(socketp);

      /*
       * c-ares sockets call neither OPENSOCKET_FUNCTION nor
       * CLOSESOCKET_FUNCTION, so we have to cleanup socket_info created in
       * register_cares_socket()
       */
      if ((*si)->is_cares_socket())
        self->socket_cleanup((*si)->socket->native_handle());

      self->monitor_socket(*si, CURL_POLL_NONE);
      delete si;
    }
  } else if (socketp) {
    // change direction
    auto* si = static_cast<socket_info_ptr*>(socketp);
    (*si)->handle = easy::from_native(native_easy);
    self->monitor_socket(*si, what);
  } else if (native_easy) {
    // register the socket
    socket_info_ptr si = self->get_socket_from_native(s);
    if (!si) {
      throw std::logic_error("si == nullptr");
    }
    si->handle = easy::from_native(native_easy);
    self->assign(s, new socket_info_ptr(si));
    self->monitor_socket(si, what);
  } else {
    throw std::invalid_argument("neither socketp nor native_easy were set");
  }

  return 0;
}

int multi::timer(native::CURLM*, [[maybe_unused]] long timeout_ms,
                 void* userp) {
  [[maybe_unused]] auto* self = static_cast<multi*>(userp);
  LOG_TRACE() << "timer " << reinterpret_cast<long>(self) << " " << timeout_ms;

  const auto& pimpl = self->pimpl_;

  LOG_TRACE() << "mult::timer (1)";
  pimpl->timer_.Cancel();
  LOG_TRACE() << "mult::timer (2)";

  if (timeout_ms > 0) {
    pimpl->timer_.SingleshotAsync(
        std::chrono::milliseconds(timeout_ms),
        std::bind(&multi::handle_timeout, self, std::placeholders::_1));
  } else if (timeout_ms == 0) {
    pimpl->timer_zero_watcher_.Send();
  }
  LOG_TRACE() << "mult::timer (3)";

  return 0;
}

void multi::handle_async() {
  pimpl_->timer_zero_watcher_.Start();
  LOG_TRACE() << "multi::handle_async()";
  handle_timeout(std::error_code());
}

}  // namespace curl
