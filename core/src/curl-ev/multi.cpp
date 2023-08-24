/**
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        Integration of libcurl's multi interface with Boost.Asio
*/

#include <cstring>
#include <string_view>
#include <system_error>

#include <curl-ev/easy.hpp>
#include <curl-ev/error_code.hpp>
#include <curl-ev/multi.hpp>
#include <curl-ev/socket_info.hpp>
#include <curl-ev/wrappers.hpp>

#include <crypto/openssl.hpp>
#include <engine/ev/watcher/async_watcher.hpp>
#include <engine/ev/watcher/timer_watcher.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace curl {

namespace {

const char* GetSetterName(native::CURLMoption option) {
  switch (option) {
    case native::CURLMOPT_PIPELINING:
      return "SetMultiplexingEnabled";
    case native::CURLMOPT_MAX_HOST_CONNECTIONS:
      return "SetMaxHostConnections";
    case native::CURLMOPT_MAXCONNECTS:
      return "SetConnectionCacheSize";
    default:
      return "<unknown setter>";
  }
}

}  // namespace

using easy_set_type = std::set<easy*>;
using BusyMarker = utils::statistics::BusyMarker;

class multi::Impl final {
 public:
  Impl(engine::ev::ThreadControl& thread_control, multi& object);

  std::vector<std::unique_ptr<socket_info>> socket_infos_;

  engine::ev::AsyncWatcher timer_zero_watcher_;

  easy_set_type easy_handles_;
  engine::ev::TimerWatcher timer_;
  int still_running_{0};
};

multi::Impl::Impl(engine::ev::ThreadControl& thread_control, multi& object)
    : timer_zero_watcher_(thread_control,
                          [&object]() { object.handle_async(); }),
      timer_(thread_control) {
  impl::CurlGlobal::Init();
}

multi::multi(engine::ev::ThreadControl& thread_control,
             const std::shared_ptr<ConnectRateLimiter>& connect_rate_limiter)
    : pimpl_(std::make_unique<Impl>(thread_control, *this)),
      thread_control_(thread_control),
      connect_rate_limiter_(connect_rate_limiter) {
  LOG_TRACE() << "multi::multi";

  // Note: curl_multi_init() is blocking.
  // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
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

  // wait for async ops in loop
  thread_control_.RunInEvLoopSync([] {});

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

void multi::BindEasySocket(easy& easy_handle, native::curl_socket_t s) {
  auto* si = GetSocketInfo(s);
  UASSERT(!si->pending_read_op);
  UASSERT(!si->pending_write_op);
  UASSERT(!si->handle);
  si->handle = &easy_handle;
}

void multi::UnbindEasySocket(native::curl_socket_t s) {
  auto* si = GetSocketInfo(s);
  UASSERT(!si->pending_read_op);
  UASSERT(!si->pending_write_op);
  si->handle = nullptr;
}

void multi::CheckRateLimit(const char* url_str, std::error_code& ec) {
  connect_rate_limiter_->Check(url_str, ec);
}

void multi::SetMultiplexingEnabled(bool value) {
  SetOptionAsync(native::CURLMOPT_PIPELINING, value ? 1 : 0);
}

void multi::SetMaxHostConnections(long value) {
  SetOptionAsync(native::CURLMOPT_MAX_HOST_CONNECTIONS, value);
}

void multi::SetConnectionCacheSize(long value) {
  SetOptionAsync(native::CURLMOPT_MAXCONNECTS, value);
}

void multi::add_handle(native::CURL* native_easy) {
  std::error_code ec{static_cast<errc::MultiErrorCode>(
      native::curl_multi_add_handle(handle_, native_easy))};
  throw_error(ec, "add_handle");
}

void multi::remove_handle(native::CURL* native_easy) {
  std::error_code ec{static_cast<errc::MultiErrorCode>(
      native::curl_multi_remove_handle(handle_, native_easy))};
  throw_error(ec, "remove_handle");
}

void multi::assign(native::curl_socket_t sockfd, void* user_data) {
  std::error_code ec{static_cast<errc::MultiErrorCode>(
      native::curl_multi_assign(handle_, sockfd, user_data))};
  throw_error(ec, "multi_assign");
}

void multi::socket_action(native::curl_socket_t s, int event_bitmask) {
  std::error_code ec{
      static_cast<errc::MultiErrorCode>(native::curl_multi_socket_action(
          handle_, s, event_bitmask, &pimpl_->still_running_))};
  throw_error(ec, "socket_action");

  if (!still_running()) {
    pimpl_->timer_.Cancel();
  }
}

void multi::set_socket_function(socket_function_t socket_function) {
  std::error_code ec{
      static_cast<errc::MultiErrorCode>(native::curl_multi_setopt(
          handle_, native::CURLMOPT_SOCKETFUNCTION, socket_function))};
  throw_error(ec, "set_socket_function");
}

void multi::set_socket_data(void* socket_data) {
  std::error_code ec{
      static_cast<errc::MultiErrorCode>(native::curl_multi_setopt(
          handle_, native::CURLMOPT_SOCKETDATA, socket_data))};
  throw_error(ec, "set_socket_data");
}

void multi::set_timer_function(timer_function_t timer_function) {
  std::error_code ec{
      static_cast<errc::MultiErrorCode>(native::curl_multi_setopt(
          handle_, native::CURLMOPT_TIMERFUNCTION, timer_function))};
  throw_error(ec, "set_timer_function");
}

void multi::set_timer_data(void* timer_data) {
  std::error_code ec{
      static_cast<errc::MultiErrorCode>(native::curl_multi_setopt(
          handle_, native::CURLMOPT_TIMERDATA, timer_data))};
  throw_error(ec, "set_timer_data");
}

void multi::SetOptionAsync(native::CURLMoption option, long value) {
  GetThreadControl().RunInEvLoopAsync([this, option, value] {
    std::error_code ec{static_cast<errc::MultiErrorCode>(
        native::curl_multi_setopt(handle_, option, value))};
    if (ec) {
      LOG_ERROR() << GetSetterName(option) << " failed: " << ec.message();
    }
  });
}

void multi::monitor_socket(socket_info* si, int action) {
  si->monitor_read = !!(action & CURL_POLL_IN);
  si->monitor_write = !!(action & CURL_POLL_OUT);

  if (!si->watcher.HasFd()) {
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

  if (action == CURL_POLL_NONE &&
      (si->pending_read_op || si->pending_write_op)) {
    si->watcher.Cancel();
    // in case watchers weren't active
    si->pending_read_op = false;
    si->pending_write_op = false;
  }
}

void multi::process_messages() {
  native::CURLMsg* msg = nullptr;
  int msgs_left = 0;

  while ((msg = native::curl_multi_info_read(handle_, &msgs_left))) {
    if (msg->msg == native::CURLMSG_DONE) {
      easy* easy_handle = easy::from_native(msg->easy_handle);
      std::error_code ec;

      if (msg->data.result != native::CURLE_OK) {
        ec =
            std::error_code{static_cast<errc::EasyErrorCode>(msg->data.result)};
      }

      remove(easy_handle);
      LOG_TRACE() << "mult::process_messages() handle_completion";
      easy_handle->handle_completion(ec);
    }
  }
}

bool multi::still_running() { return (pimpl_->still_running_ > 0); }

void multi::start_read_op(socket_info* si) {
  LOG_TRACE() << "start_read_op si=" << si;

  si->pending_read_op = true;

  si->watcher.ReadAsync(
      [this, si](std::error_code err) { handle_socket_read(err, si); });
}

void multi::handle_socket_read(std::error_code err, socket_info* si) {
  LOG_TRACE() << "handle_socket_read si=" << si << " ec=" << err
              << " watcher=" << &si->watcher;
  si->pending_read_op = false;
  if (!si->watcher.HasFd()) return;
  if (!err) {
    socket_action(si->watcher.GetFd(), CURL_CSELECT_IN);
    process_messages();

    // read could've been already restarted by socket_action
    if (si->monitor_read && !si->pending_read_op) start_read_op(si);
  } else if (err != std::make_error_code(std::errc::operation_canceled)) {
    socket_action(si->watcher.GetFd(), CURL_CSELECT_ERR);
    process_messages();
  }
}

void multi::start_write_op(socket_info* si) {
  LOG_TRACE() << "start_write_op si=" << si;
  si->pending_write_op = true;

  si->watcher.WriteAsync(
      [this, si](std::error_code err) { handle_socket_write(err, si); });
}

void multi::handle_socket_write(std::error_code err, socket_info* si) {
  LOG_TRACE() << "handle_socket_write si=" << si << " ec=" << err
              << " watcher=" << &si->watcher;
  si->pending_write_op = false;
  if (!si->watcher.HasFd()) return;

  BusyMarker busy(Statistics().get_busy_storage());

  if (!err) {
    socket_action(si->watcher.GetFd(), CURL_CSELECT_OUT);
    process_messages();

    // write could've been already restarted by socket_action
    if (si->monitor_write && !si->pending_write_op) start_write_op(si);
  } else if (err != std::make_error_code(std::errc::operation_canceled)) {
    socket_action(si->watcher.GetFd(), CURL_CSELECT_ERR);
    process_messages();
  }
}

void multi::handle_timeout(const std::error_code& err) {
  if (!err) {
    BusyMarker busy(Statistics().get_busy_storage());
    LOG_TRACE() << "handle_timeout " << this;
    socket_action(CURL_SOCKET_TIMEOUT, 0);
    process_messages();
  }
}

socket_info* multi::GetSocketInfo(native::curl_socket_t fd) {
  if (fd == -1) return {};
  UASSERT(fd >= 0);

  if (pimpl_->socket_infos_.size() <= static_cast<size_t>(fd)) {
    pimpl_->socket_infos_.resize(std::max(fd + 1, fd * 2));
  }

  auto& si = pimpl_->socket_infos_[fd];
  if (!si) {
    si = std::make_unique<socket_info>(GetThreadControl());
  }
  return si.get();
}

// multi::assign() may throw when used on an invalid socket, shouldn't happen
int multi::socket(native::CURL*, native::curl_socket_t s, int what, void* userp,
                  void* socketp) noexcept {
  auto* self = static_cast<multi*>(userp);
  LOG_TRACE() << "socket multi=" << self << " what=" << what
              << " si=" << socketp;

  if (what == CURL_POLL_REMOVE) {
    // stop listening for events
    if (socketp) {
      auto* si = static_cast<socket_info*>(socketp);
      self->monitor_socket(si, CURL_POLL_NONE);
      self->assign(s, nullptr);
      si->watcher.Release();
    }
  } else if (socketp) {
    // change direction
    auto* si = static_cast<socket_info*>(socketp);
    self->monitor_socket(si, what);
  } else {
    // register the socket
    auto* si = self->GetSocketInfo(s);
    UASSERT(si);
    UASSERT(!si->watcher.HasFd());
    si->watcher.SetFd(s);
    self->assign(s, si);
    self->monitor_socket(si, what);
  }

  return 0;
}

int multi::timer(native::CURLM*, [[maybe_unused]] long timeout_ms,
                 void* userp) noexcept {
  [[maybe_unused]] auto* self = static_cast<multi*>(userp);
  LOG_TRACE() << "timer " << self << " " << timeout_ms;

  const auto& pimpl = self->pimpl_;

  LOG_TRACE() << "mult::timer (1)";
  pimpl->timer_.Cancel();
  LOG_TRACE() << "mult::timer (2)";

  if (timeout_ms > 0) {
    pimpl->timer_.SingleshotAsync(
        std::chrono::milliseconds(timeout_ms),
        [self](std::error_code err) { self->handle_timeout(err); });
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

USERVER_NAMESPACE_END
