/**
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        C++ wrapper for libcurl's easy interface
*/

#include <sys/socket.h>
#include <unistd.h>

#include <utility>

#include <curl-ev/easy.hpp>
#include <curl-ev/error_code.hpp>
#include <curl-ev/form.hpp>
#include <curl-ev/multi.hpp>
#include <curl-ev/share.hpp>
#include <curl-ev/string_list.hpp>
#include <curl-ev/wrappers.hpp>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <engine/ev/thread_control.hpp>
#include <server/net/listener_impl.hpp>
#include <userver/engine/async.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/str_icase.hpp>
#include <utils/strerror.hpp>

USERVER_NAMESPACE_BEGIN

namespace curl {
namespace {

bool IsHeaderMatchingName(std::string_view header, std::string_view name) {
  return header.size() > name.size() &&
         utils::StrIcaseEqual()(header.substr(0, name.size()), name) &&
         (header[name.size()] == ':' || header[name.size()] == ';');
}

std::optional<std::string_view> FindHeaderByNameImpl(
    const std::shared_ptr<string_list>& headers, std::string_view name) {
  if (!headers) return std::nullopt;
  auto result = headers->FindIf([name](std::string_view header) {
    return IsHeaderMatchingName(header, name);
  });
  if (result) {
    result->remove_prefix(name.size() + 1);
    while (!result->empty() && result->front() == ' ') result->remove_prefix(1);
  }
  return result;
}

fmt::memory_buffer CreateHeaderBuffer(std::string_view name,
                                      std::string_view value,
                                      easy::EmptyHeaderAction action) {
  fmt::memory_buffer buf;

  if (action == easy::EmptyHeaderAction::kSend && value.empty()) {
    fmt::format_to(std::back_inserter(buf), FMT_COMPILE("{};"), name);
  } else {
    fmt::format_to(std::back_inserter(buf), FMT_COMPILE("{}: {}"), name, value);
  }

  buf.push_back('\0');

  return buf;
}

bool AddHeaderDoSkip(const std::shared_ptr<string_list>& headers,
                     std::string_view name,
                     easy::DuplicateHeaderAction action) {
  if (action == easy::DuplicateHeaderAction::kSkip && headers) {
    if (FindHeaderByNameImpl(headers, name)) return true;
  }

  return false;
}

bool AddHeaderDoReplace(const std::shared_ptr<string_list>& headers,
                        const fmt::memory_buffer& buf, std::string_view name,
                        easy::DuplicateHeaderAction action) {
  if (action == easy::DuplicateHeaderAction::kReplace && headers) {
    const bool replaced = headers->ReplaceFirstIf(
        [name](std::string_view header) {
          return IsHeaderMatchingName(header, name);
        },
        buf.data());

    if (replaced) return true;
  }

  return false;
}

}  // namespace

using BusyMarker = utils::statistics::BusyMarker;

easy::easy(native::CURL* easy_handle, multi* multi_handle)
    : handle_(easy_handle),
      multi_(multi_handle),
      construct_ts_(std::chrono::steady_clock::now()) {
  UASSERT(handle_);
  set_private(this);
}

easy::~easy() {
  cancel();

  if (handle_) {
    native::curl_easy_cleanup(handle_);
    handle_ = nullptr;
  }
}

std::shared_ptr<const easy> easy::CreateBlocking() {
  impl::CurlGlobal::Init();

  // Note: curl_easy_init() is blocking.
  auto* handle = native::curl_easy_init();
  if (!handle) {
    throw std::bad_alloc();
  }

  return std::make_shared<const easy>(handle, nullptr);
}

std::shared_ptr<easy> easy::GetBoundBlocking(multi& multi_handle) const {
  // Note: curl_easy_init() is blocking.
  auto* cloned = native::curl_easy_duphandle(handle_);
  if (!cloned) {
    throw std::bad_alloc();
  }

  return std::make_shared<easy>(cloned, &multi_handle);
}

easy* easy::from_native(native::CURL* native_easy) {
  easy* easy_handle = nullptr;
  native::curl_easy_getinfo(native_easy, native::CURLINFO_PRIVATE,
                            &easy_handle);
  return easy_handle;
}

engine::ev::ThreadControl& easy::GetThreadControl() {
  return multi_->GetThreadControl();
}

void easy::async_perform(handler_type handler) {
  LOG_TRACE() << "easy::async_perform start " << this;
  size_t request_num = ++request_counter_;
  if (multi_) {
    multi_->GetThreadControl().RunInEvLoopDeferred(
        [self = shared_from_this(), this, handler = std::move(handler),
         request_num]() mutable {
          return do_ev_async_perform(std::move(handler), request_num);
        });
  } else {
    throw std::runtime_error("no multi!");
  }
  LOG_TRACE() << "easy::async_perform finished " << this;
}

void easy::do_ev_async_perform(handler_type handler, size_t request_num) {
  if (request_num <= cancelled_request_max_) {
    LOG_DEBUG() << "already cancelled";
    return;
  }

  LOG_TRACE() << "easy::do_ev_async_perform start " << this;
  mark_start_performing();
  if (!multi_) {
    throw std::runtime_error(
        "attempt to perform async. operation without assigning a multi object");
  }

  BusyMarker busy(multi_->Statistics().get_busy_storage());

  // Cancel all previous async. operations
  cancel(request_num - 1);

  // Keep track of all new sockets
  set_opensocket_function(&easy::opensocket);
  set_opensocket_data(this);

  // This one is tricky: Although sockets are opened in the context of an easy
  // object, they can outlive the easy objects and be transferred into a multi
  // object's connection pool. Why there is no connection pool interface in the
  // multi interface to plug into to begin with is still a mystery to me. Either
  // way, the close events have to be tracked by the multi object as sockets are
  // usually closed when curl_multi_cleanup is invoked.
  set_closesocket_function(&easy::closesocket);
  set_closesocket_data(multi_);

  handler_ = std::move(handler);
  multi_registered_ = true;

  // Registering the easy handle with the multi handle might invoke a set of
  // callbacks right away which cause the completion event to fire from within
  // this function.
  LOG_TRACE() << "easy::do_ev_async_perform before multi_->add() " << this;
  multi_->add(this);
}

void easy::cancel() { cancel(request_counter_); }

void easy::cancel(size_t request_num) {
  if (multi_) {
    multi_->GetThreadControl().RunInEvLoopSync(
        [this, request_num] { do_ev_cancel(request_num); });
  }
}

void easy::do_ev_cancel(size_t request_num) {
  // RunInEvLoopAsync(do_ev_async_perform) and RunInEvLoopSync(do_ev_cancel) are
  // not synchronized. So we need to count last cancelled request to prevent its
  // execution in do_ev_async_perform().
  if (cancelled_request_max_ < request_num)
    cancelled_request_max_ = request_num;
  if (multi_registered_) {
    BusyMarker busy(multi_->Statistics().get_busy_storage());

    handle_completion(std::make_error_code(std::errc::operation_canceled));
    multi_->remove(this);
  }
}

void easy::reset() {
  LOG_TRACE() << "easy::reset start " << this;

  orig_url_str_.clear();
  std::string{}.swap(post_fields_);  // forced memory freeing
  form_.reset();
  if (headers_) headers_->clear();
  if (proxy_headers_) proxy_headers_->clear();
  if (http200_aliases_) http200_aliases_->clear();
  if (resolved_hosts_) resolved_hosts_->clear();
  share_.reset();
  retries_count_ = 0;
  sockets_opened_ = 0;
  rate_limit_error_.clear();

  set_custom_request(nullptr);
  set_no_body(false);
  set_post(false);

  // MAC_COMPAT: Secure Transport does not provide these
  std::error_code ec;
  set_ssl_ctx_data(nullptr, ec);
  if (!ec) {
    set_ssl_ctx_function(nullptr);
  } else if (ec != errc::EasyErrorCode::kNotBuiltIn) {
    throw_error(ec, "set_ssl_ctx_data");
  }

  UASSERT(!multi_registered_);
  native::curl_easy_reset(handle_);
  set_private(this);

  LOG_TRACE() << "easy::reset finished " << this;
}

void easy::mark_start_performing() {
  if (start_performing_ts_ == time_point{}) {
    start_performing_ts_ = std::chrono::steady_clock::now();
  }
}
void easy::mark_open_socket() { ++sockets_opened_; }

void easy::set_source(std::shared_ptr<std::istream> source) {
  std::error_code ec;
  set_source(std::move(source), ec);
  throw_error(ec, "set_source");
}

void easy::set_source(std::shared_ptr<std::istream> source,
                      std::error_code& ec) {
  source_ = std::move(source);
  set_read_function(&easy::read_function, ec);
  if (!ec) set_read_data(this, ec);
  if (!ec) set_seek_function(&easy::seek_function, ec);
  if (!ec) set_seek_data(this, ec);
}

void easy::set_sink(std::string* sink) {
  std::error_code ec;
  set_sink(sink, ec);
  throw_error(ec, "set_sink");
}

size_t easy::header_function(void*, size_t size, size_t nmemb, void*) {
  return size * nmemb;
}

void easy::set_sink(std::string* sink, std::error_code& ec) {
  sink_ = sink;
  set_write_function(&easy::write_function);
  if (!ec) set_write_data(this);
}

void easy::unset_progress_callback() {
  set_no_progress(true);
  set_xferinfo_function(nullptr);
  set_xferinfo_data(nullptr);
}

void easy::set_progress_callback(progress_callback_t progress_callback) {
  progress_callback_ = std::move(progress_callback);
  set_no_progress(false);
  set_xferinfo_function(&easy::xferinfo_function);
  set_xferinfo_data(this);
}

void easy::set_url(std::string url_str) {
  std::error_code ec;
  set_url(std::move(url_str), ec);
  throw_error(ec, "set_url");
}

void easy::set_url(std::string url_str, std::error_code& ec) {
  orig_url_str_ = std::move(url_str);
  url_.SetAbsoluteUrl(orig_url_str_.c_str(), ec);
  if (!ec) {
    ec = static_cast<errc::EasyErrorCode>(native::curl_easy_setopt(
        handle_, native::CURLOPT_CURLU, url_.native_handle()));
    UASSERT(!ec);
  }
  // not else, catch all errors
  if (ec) {
    orig_url_str_.clear();
  }
}

const std::string& easy::get_original_url() const { return orig_url_str_; }

const url& easy::get_easy_url() const { return url_; }

void easy::set_post_fields(std::string&& post_fields) {
  std::error_code ec;
  set_post_fields(std::move(post_fields), ec);
  throw_error(ec, "set_post_fields");
}

void easy::set_post_fields(std::string&& post_fields, std::error_code& ec) {
  post_fields_ = std::move(post_fields);
  ec =
      std::error_code{static_cast<errc::EasyErrorCode>(native::curl_easy_setopt(
          handle_, native::CURLOPT_POSTFIELDS, post_fields_.c_str()))};

  if (!ec)
    set_post_field_size_large(
        static_cast<native::curl_off_t>(post_fields_.length()), ec);
}

void easy::set_http_post(std::shared_ptr<form> form) {
  std::error_code ec;
  set_http_post(std::move(form), ec);
  throw_error(ec, "set_http_post");
}

void easy::set_http_post(std::shared_ptr<form> form, std::error_code& ec) {
  form_ = std::move(form);

  if (form_) {
    ec = std::error_code{
        static_cast<errc::EasyErrorCode>(native::curl_easy_setopt(
            handle_, native::CURLOPT_HTTPPOST, form_->native_handle()))};
  } else {
    ec = std::error_code{static_cast<errc::EasyErrorCode>(
        native::curl_easy_setopt(handle_, native::CURLOPT_HTTPPOST, NULL))};
  }
}

void easy::add_header(std::string_view name, std::string_view value,
                      EmptyHeaderAction empty_header_action,
                      DuplicateHeaderAction duplicate_header_action) {
  std::error_code ec;
  add_header(name, value, ec, empty_header_action, duplicate_header_action);
  throw_error(ec, "add_header");
}

void easy::add_header(std::string_view name, std::string_view value,
                      std::error_code& ec,
                      EmptyHeaderAction empty_header_action,
                      DuplicateHeaderAction duplicate_header_action) {
  if (AddHeaderDoSkip(headers_, name, duplicate_header_action)) {
    return;
  }

  auto buf = CreateHeaderBuffer(name, value, empty_header_action);

  if (AddHeaderDoReplace(headers_, buf, name, duplicate_header_action)) {
    return;
  }

  add_header(buf.data(), ec);
}

void easy::add_header(std::string_view name, std::string_view value,
                      DuplicateHeaderAction duplicate_header_action) {
  std::error_code ec;
  add_header(name, value, ec, duplicate_header_action);
  throw_error(ec, "add_header");
}

void easy::add_header(std::string_view name, std::string_view value,
                      std::error_code& ec,
                      DuplicateHeaderAction duplicate_header_action) {
  add_header(name, value, ec, EmptyHeaderAction::kSend,
             duplicate_header_action);
}

std::optional<std::string_view> easy::FindHeaderByName(
    std::string_view name) const {
  return FindHeaderByNameImpl(headers_, name);
}

void easy::add_header(const char* header) {
  std::error_code ec;
  add_header(header, ec);
  throw_error(ec, "add_header");
}

void easy::add_header(const char* header, std::error_code& ec) {
  if (!headers_) {
    headers_ = std::make_shared<string_list>();
  }

  headers_->add(header);
  ec =
      std::error_code{static_cast<errc::EasyErrorCode>(native::curl_easy_setopt(
          handle_, native::CURLOPT_HTTPHEADER, headers_->native_handle()))};
}

void easy::add_header(const std::string& header) { add_header(header.c_str()); }

void easy::add_header(const std::string& header, std::error_code& ec) {
  add_header(header.c_str(), ec);
}

void easy::set_headers(std::shared_ptr<string_list> headers) {
  std::error_code ec;
  set_headers(std::move(headers), ec);
  throw_error(ec, "set_headers");
}

void easy::set_headers(std::shared_ptr<string_list> headers,
                       std::error_code& ec) {
  headers_ = std::move(headers);

  if (headers_) {
    ec = std::error_code{
        static_cast<errc::EasyErrorCode>(native::curl_easy_setopt(
            handle_, native::CURLOPT_HTTPHEADER, headers_->native_handle()))};
  } else {
    ec = std::error_code{static_cast<errc::EasyErrorCode>(
        native::curl_easy_setopt(handle_, native::CURLOPT_HTTPHEADER, NULL))};
  }
}

void easy::add_proxy_header(std::string_view name, std::string_view value,
                            EmptyHeaderAction empty_header_action,
                            DuplicateHeaderAction duplicate_header_action) {
  std::error_code ec;
  add_proxy_header(name, value, ec, empty_header_action,
                   duplicate_header_action);
  throw_error(ec, "add_proxy_header");
}

void easy::add_proxy_header(std::string_view name, std::string_view value,
                            std::error_code& ec,
                            EmptyHeaderAction empty_header_action,
                            DuplicateHeaderAction duplicate_header_action) {
  if (AddHeaderDoSkip(proxy_headers_, name, duplicate_header_action)) {
    return;
  }

  auto buf = CreateHeaderBuffer(name, value, empty_header_action);

  if (AddHeaderDoReplace(proxy_headers_, buf, name, duplicate_header_action)) {
    return;
  }

  add_proxy_header(buf.data(), ec);
}

void easy::add_proxy_header(const char* header, std::error_code& ec) {
  if (!proxy_headers_) {
    proxy_headers_ = std::make_shared<string_list>();
  }

  proxy_headers_->add(header);
  ec = std::error_code{static_cast<errc::EasyErrorCode>(
      native::curl_easy_setopt(handle_, native::CURLOPT_PROXYHEADER,
                               proxy_headers_->native_handle()))};
}

void easy::add_http200_alias(const std::string& http200_alias) {
  std::error_code ec;
  add_http200_alias(http200_alias, ec);
  throw_error(ec, "add_http200_alias");
}

void easy::add_http200_alias(const std::string& http200_alias,
                             std::error_code& ec) {
  if (!http200_aliases_) {
    http200_aliases_ = std::make_shared<string_list>();
  }

  http200_aliases_->add(http200_alias);
  ec = std::error_code{static_cast<errc::EasyErrorCode>(
      native::curl_easy_setopt(handle_, native::CURLOPT_HTTP200ALIASES,
                               http200_aliases_->native_handle()))};
}

void easy::set_http200_aliases(std::shared_ptr<string_list> http200_aliases) {
  std::error_code ec;
  set_http200_aliases(std::move(http200_aliases), ec);
  throw_error(ec, "set_http200_aliases");
}

void easy::set_http200_aliases(std::shared_ptr<string_list> http200_aliases,
                               std::error_code& ec) {
  http200_aliases_ = std::move(http200_aliases);

  if (http200_aliases) {
    ec = std::error_code{static_cast<errc::EasyErrorCode>(
        native::curl_easy_setopt(handle_, native::CURLOPT_HTTP200ALIASES,
                                 http200_aliases_->native_handle()))};
  } else {
    ec = std::error_code{
        static_cast<errc::EasyErrorCode>(native::curl_easy_setopt(
            handle_, native::CURLOPT_HTTP200ALIASES, nullptr))};
  }
}

void easy::add_resolve(const std::string& host, const std::string& port,
                       const std::string& addr) {
  std::error_code ec;
  add_resolve(host, port, addr, ec);
  throw_error(ec, "add_resolve");
}

void easy::add_resolve(const std::string& host, const std::string& port,
                       const std::string& addr, std::error_code& ec) {
  if (!resolved_hosts_) {
    resolved_hosts_ = std::make_shared<string_list>();
  }
  auto host_port_addr = utils::StrCat(host, ":", port, ":", addr);
  const std::string_view host_port_view{host_port_addr.data(),
                                        host_port_addr.size() - addr.size()};

  if (!resolved_hosts_->ReplaceFirstIf(
          [host_port_view](const auto& entry) {
            // host_port_addr, of which we hold a string_view, might be moved in
            // ReplaceFirstIf, but it's guaranteed that this
            // lambda is not called after that.
            return std::string_view{entry}.substr(host_port_view.size()) ==
                   host_port_view;
          },
          std::move(host_port_addr))) {
    UASSERT_MSG(
        !host_port_addr.empty(),
        "ReplaceFirstIf moved the string out, when it shouldn't have done so.");
    resolved_hosts_->add(std::move(host_port_addr));
  }

  ec =
      std::error_code{static_cast<errc::EasyErrorCode>(native::curl_easy_setopt(
          handle_, native::CURLOPT_RESOLVE, resolved_hosts_->native_handle()))};
}

void easy::set_resolves(std::shared_ptr<string_list> resolved_hosts) {
  std::error_code ec;
  set_resolves(std::move(resolved_hosts), ec);
  throw_error(ec, "set_resolves");
}

void easy::set_resolves(std::shared_ptr<string_list> resolved_hosts,
                        std::error_code& ec) {
  resolved_hosts_ = std::move(resolved_hosts);

  if (resolved_hosts_) {
    ec = std::error_code{static_cast<errc::EasyErrorCode>(
        native::curl_easy_setopt(handle_, native::CURLOPT_RESOLVE,
                                 resolved_hosts_->native_handle()))};
  } else {
    ec = std::error_code{static_cast<errc::EasyErrorCode>(
        native::curl_easy_setopt(handle_, native::CURLOPT_RESOLVE, NULL))};
  }
}

void easy::set_share(std::shared_ptr<share> share) {
  std::error_code ec;
  set_share(std::move(share), ec);
  throw_error(ec, "set_share");
}

void easy::set_share(std::shared_ptr<share> share, std::error_code& ec) {
  share_ = std::move(share);

  if (share) {
    ec = std::error_code{
        static_cast<errc::EasyErrorCode>(native::curl_easy_setopt(
            handle_, native::CURLOPT_SHARE, share_->native_handle()))};
  } else {
    ec = std::error_code{static_cast<errc::EasyErrorCode>(
        native::curl_easy_setopt(handle_, native::CURLOPT_SHARE, NULL))};
  }
}

bool easy::has_post_data() const { return !post_fields_.empty() || form_; }

const std::string& easy::get_post_data() const { return post_fields_; }

std::string easy::extract_post_data() {
  auto data = std::move(post_fields_);
  set_post_fields({});
  return data;
}

void easy::handle_completion(const std::error_code& err) {
  LOG_TRACE() << "easy::handle_completion easy=" << this;

  multi_registered_ = false;

  auto handler = std::function<void(std::error_code)>([](std::error_code) {});
  swap(handler, handler_);

  /* It's OK to call handler in libev thread context as it is limited to
   * Request::on_retry and Request::on_completed. All user code is executed in
   * coro context.
   */
  handler(err);
}

void easy::mark_retry() { ++retries_count_; }

clients::http::LocalStats easy::get_local_stats() {
  clients::http::LocalStats stats;

  stats.open_socket_count = sockets_opened_;
  stats.retries_count = retries_count_;
  stats.time_to_connect = std::chrono::microseconds(get_connect_time_usec());
  stats.time_to_process = std::chrono::microseconds(get_total_time_usec());

  return stats;
}

std::error_code easy::rate_limit_error() const { return rate_limit_error_; }

easy::time_point::duration easy::time_to_start() const {
  if (start_performing_ts_ != time_point{}) {
    return start_performing_ts_ - construct_ts_;
  }
  return {};
}

native::curl_socket_t easy::open_tcp_socket(native::curl_sockaddr* address) {
  std::error_code ec;

  LOG_TRACE() << "open_tcp_socket family=" << address->family;

  int fd = socket(address->family, address->socktype, address->protocol);
  if (fd == -1) {
    const auto old_errno = errno;
    LOG_ERROR() << "socket(2) failed with error: "
                << utils::strerror(old_errno);
    return CURL_SOCKET_BAD;
  }
  if (multi_) multi_->BindEasySocket(*this, fd);
  return fd;
}

size_t easy::write_function(char* ptr, size_t size, size_t nmemb,
                            void* userdata) noexcept {
  easy* self = static_cast<easy*>(userdata);
  size_t actual_size = size * nmemb;

  if (!actual_size) {
    return 0;
  }

  try {
    self->sink_->append(ptr, actual_size);
  } catch (const std::exception&) {
    // out of memory
    return 0;
  }

  return actual_size;
}

size_t easy::read_function(void* ptr, size_t size, size_t nmemb,
                           void* userdata) noexcept {
  // FIXME readsome doesn't work with TFTP (see cURL docs)

  easy* self = static_cast<easy*>(userdata);
  size_t actual_size = size * nmemb;

  if (!self->source_) return CURL_READFUNC_ABORT;

  if (self->source_->eof()) {
    return 0;
  }

  std::streamsize chars_stored =
      self->source_->readsome(static_cast<char*>(ptr), actual_size);

  if (!*self->source_) {
    return CURL_READFUNC_ABORT;
  } else {
    return static_cast<size_t>(chars_stored);
  }
}

int easy::seek_function(void* instream, native::curl_off_t offset,
                        int origin) noexcept {
  // TODO we could allow the user to define an offset which this library
  // should consider as position zero for uploading chunks of the file
  // alternatively do tellg() on the source stream when it is first passed to
  // use_source() and use that as origin

  easy* self = static_cast<easy*>(instream);

  std::ios::seekdir dir = std::ios::beg;

  switch (origin) {
    case SEEK_SET:
      dir = std::ios::beg;
      break;

    case SEEK_CUR:
      dir = std::ios::cur;
      break;

    case SEEK_END:
      dir = std::ios::end;
      break;

    default:
      return CURL_SEEKFUNC_FAIL;
  }

  if (!self->source_->seekg(offset, dir)) {
    return CURL_SEEKFUNC_FAIL;
  } else {
    return CURL_SEEKFUNC_OK;
  }
}

int easy::xferinfo_function(void* clientp, native::curl_off_t dltotal,
                            native::curl_off_t dlnow,
                            native::curl_off_t ultotal,
                            native::curl_off_t ulnow) noexcept {
  easy* self = static_cast<easy*>(clientp);
  try {
    return self->progress_callback_(dltotal, dlnow, ultotal, ulnow) ? 0 : 1;
  } catch (const std::exception& ex) {
    LOG_LIMITED_WARNING() << "Progress callback failed: " << ex;
    return 1;
  }
}

native::curl_socket_t easy::opensocket(
    void* clientp, native::curlsocktype purpose,
    struct native::curl_sockaddr* address) noexcept {
  easy* self = static_cast<easy*>(clientp);
  multi* multi_handle = self->multi_;
  native::curl_socket_t s = -1;

  if (multi_handle) {
    std::error_code ec;
    auto url = self->get_effective_url(ec);
    if (ec || url.empty()) {
      LOG_DEBUG() << "Cannot get effective url: " << ec;
      UASSERT_MSG(false, "Cannot get effective url: " + ec.message());
    }

    multi_handle->CheckRateLimit(url.data(), self->rate_limit_error_);
    if (self->rate_limit_error_) {
      multi_handle->Statistics().mark_socket_ratelimited();
      return CURL_SOCKET_BAD;
    } else {
      LOG_TRACE() << "not throttled";
    }
  } else {
    LOG_TRACE() << "skip throttle check";
  }

  if (purpose == native::CURLSOCKTYPE_IPCXN &&
      address->socktype == SOCK_STREAM) {
    // Note to self: Why is address->protocol always set to zero?
    s = self->open_tcp_socket(address);
    if (s != -1 && multi_handle) {
      multi_handle->Statistics().mark_open_socket();
      self->mark_open_socket();
    }
    return s;
  }

  // unknown or invalid socket type
  return CURL_SOCKET_BAD;
}

int easy::closesocket(void* clientp, native::curl_socket_t item) noexcept {
  auto* multi_handle = static_cast<multi*>(clientp);
  multi_handle->UnbindEasySocket(item);

  int ret = close(item);
  if (ret == -1) {
    const auto old_errno = errno;
    LOG_ERROR() << "close(2) failed with error: " << utils::strerror(old_errno);
  }

  multi_handle->Statistics().mark_close_socket();
  return 0;
}

}  // namespace curl

USERVER_NAMESPACE_END
