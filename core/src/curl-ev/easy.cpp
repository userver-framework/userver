/**
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        C++ wrapper for libcurl's easy interface
*/

#include <utility>

#include <curl-ev/easy.hpp>
#include <curl-ev/error_code.hpp>
#include <curl-ev/form.hpp>
#include <curl-ev/multi.hpp>
#include <curl-ev/share.hpp>
#include <curl-ev/socket_info.hpp>
#include <curl-ev/string_list.hpp>

#include <engine/async.hpp>
#include <server/net/listener_impl.hpp>

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace curl;
using BusyMarker = ::utils::statistics::BusyMarker;

easy* easy::from_native(native::CURL* native_easy) {
  easy* easy_handle;
  native::curl_easy_getinfo(native_easy, native::CURLINFO_PRIVATE,
                            &easy_handle);
  return easy_handle;
}

engine::ev::ThreadControl& easy::GetThreadControl() {
  return multi_->GetThreadControl();
}

easy::easy(multi& multi_handle)
    : multi_(&multi_handle), multi_registered_(false) {
  init();
}

easy::~easy() {
  cancel();

  if (handle_) {
    native::curl_easy_cleanup(handle_);
    handle_ = nullptr;
  }
}

void easy::async_perform(handler_type handler) {
  LOG_TRACE() << "easy::async_perform start " << reinterpret_cast<long>(this);
  size_t request_num = ++request_counter_;
  if (multi_) {
    multi_->GetThreadControl().RunInEvLoopAsync(
        [self = shared_from_this(), this, handler = std::move(handler),
         request_num]() mutable {
          return do_ev_async_perform(std::move(handler), request_num);
        });
  } else {
    throw std::runtime_error("no multi!");
  }
  LOG_TRACE() << "easy::async_perform finished "
              << reinterpret_cast<long>(this);
}

void easy::do_ev_async_perform(handler_type handler, size_t request_num) {
  if (request_num <= cancelled_request_max_) {
    LOG_DEBUG() << "already cancelled";
    return;
  }

  LOG_TRACE() << "easy::do_ev_async_perform start "
              << reinterpret_cast<long>(this);
  timings_.mark_start_performing();
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
  LOG_TRACE() << "easy::do_ev_async_perform before multi_->add() "
              << reinterpret_cast<long>(this);
  multi_->add(this);
}

void easy::cancel() { cancel(request_counter_); }

void easy::cancel(size_t request_num) {
  multi_->GetThreadControl().RunInEvLoopSync(
      std::bind(&easy::do_ev_cancel, this, request_num));
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
  LOG_TRACE() << "easy::reset start " << reinterpret_cast<long>(this);

  post_fields_.clear();
  form_.reset();
  if (headers_) headers_->clear();
  if (http200_aliases_) http200_aliases_->clear();
  if (resolved_hosts_) resolved_hosts_->clear();
  share_.reset();
  if (telnet_options_) telnet_options_->clear();

  set_custom_request(nullptr);
  set_no_body(false);
  set_post(false);

  multi_->GetThreadControl().RunInEvLoopSync(
      std::bind(&easy::do_ev_reset, this));
  LOG_TRACE() << "easy::reset finished " << reinterpret_cast<long>(this);
}

void easy::do_ev_reset() {
  if (multi_registered_) {
    native::curl_easy_reset(handle_);
  }
}

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

void easy::set_sink(std::ostream* sink) {
  std::error_code ec;
  set_sink(sink, ec);
  throw_error(ec, "set_sink");
}

size_t easy::header_function(void*, size_t size, size_t nmemb, void*) {
  return size * nmemb;
}

void easy::set_sink(std::ostream* sink, std::error_code& ec) {
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

void easy::set_post_fields(const std::string& post_fields) {
  std::error_code ec;
  set_post_fields(post_fields, ec);
  throw_error(ec, "set_post_fields");
}

void easy::set_post_fields(const std::string& post_fields,
                           std::error_code& ec) {
  post_fields_ = post_fields;
  ec = std::error_code(native::curl_easy_setopt(
      handle_, native::CURLOPT_POSTFIELDS, post_fields_.c_str()));

  if (!ec)
    set_post_field_size_large(
        static_cast<native::curl_off_t>(post_fields_.length()), ec);
}

void easy::set_post_fields(std::string&& post_fields) {
  std::error_code ec;
  set_post_fields(std::forward<std::string>(post_fields), ec);
  throw_error(ec, "set_post_fields");
}

void easy::set_post_fields(std::string&& post_fields, std::error_code& ec) {
  post_fields_ = std::move(post_fields);
  ec = std::error_code(native::curl_easy_setopt(
      handle_, native::CURLOPT_POSTFIELDS, post_fields_.c_str()));

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
    ec = std::error_code(native::curl_easy_setopt(
        handle_, native::CURLOPT_HTTPPOST, form_->native_handle()));
  } else {
    ec = std::error_code(
        native::curl_easy_setopt(handle_, native::CURLOPT_HTTPPOST, NULL));
  }
}

void easy::add_header(const std::string& name, const std::string& value) {
  std::error_code ec;
  add_header(name, value, ec);
  throw_error(ec, "add_header");
}

void easy::add_header(const std::string& name, const std::string& value,
                      std::error_code& ec) {
  add_header(name + ": " + value, ec);
}

void easy::add_header(const std::string& header) {
  std::error_code ec;
  add_header(header, ec);
  throw_error(ec, "add_header");
}

void easy::add_header(const std::string& header, std::error_code& ec) {
  if (!headers_) {
    headers_ = std::make_shared<string_list>();
  }

  headers_->add(header);
  ec = std::error_code(native::curl_easy_setopt(
      handle_, native::CURLOPT_HTTPHEADER, headers_->native_handle()));
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
    ec = std::error_code(native::curl_easy_setopt(
        handle_, native::CURLOPT_HTTPHEADER, headers_->native_handle()));
  } else {
    ec = std::error_code(
        native::curl_easy_setopt(handle_, native::CURLOPT_HTTPHEADER, NULL));
  }
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
  ec = std::error_code(
      native::curl_easy_setopt(handle_, native::CURLOPT_HTTP200ALIASES,
                               http200_aliases_->native_handle()));
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
    ec = std::error_code(
        native::curl_easy_setopt(handle_, native::CURLOPT_HTTP200ALIASES,
                                 http200_aliases_->native_handle()));
  } else {
    ec = std::error_code(native::curl_easy_setopt(
        handle_, native::CURLOPT_HTTP200ALIASES, nullptr));
  }
}

void easy::add_resolve(const std::string& resolved_host) {
  std::error_code ec;
  add_resolve(resolved_host, ec);
  throw_error(ec, "add_resolve");
}

void easy::add_resolve(const std::string& resolved_host, std::error_code& ec) {
  if (!resolved_hosts_) {
    resolved_hosts_ = std::make_shared<string_list>();
  }

  resolved_hosts_->add(resolved_host);
  ec = std::error_code(native::curl_easy_setopt(
      handle_, native::CURLOPT_RESOLVE, resolved_hosts_->native_handle()));
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
    ec = std::error_code(native::curl_easy_setopt(
        handle_, native::CURLOPT_RESOLVE, resolved_hosts_->native_handle()));
  } else {
    ec = std::error_code(
        native::curl_easy_setopt(handle_, native::CURLOPT_RESOLVE, NULL));
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
    ec = std::error_code(native::curl_easy_setopt(
        handle_, native::CURLOPT_SHARE, share_->native_handle()));
  } else {
    ec = std::error_code(
        native::curl_easy_setopt(handle_, native::CURLOPT_SHARE, NULL));
  }
}

void easy::add_telnet_option(const std::string& telnet_option) {
  std::error_code ec;
  add_telnet_option(telnet_option, ec);
  throw_error(ec, "add_telnet_option");
}

void easy::add_telnet_option(const std::string& telnet_option,
                             std::error_code& ec) {
  if (!telnet_options_) {
    telnet_options_ = std::make_shared<string_list>();
  }

  telnet_options_->add(telnet_option);
  ec = std::error_code(
      native::curl_easy_setopt(handle_, native::CURLOPT_TELNETOPTIONS,
                               telnet_options_->native_handle()));
}

void easy::add_telnet_option(const std::string& option,
                             const std::string& value) {
  std::error_code ec;
  add_telnet_option(option, value, ec);
  throw_error(ec, "add_telnet_option");
}

void easy::add_telnet_option(const std::string& option,
                             const std::string& value, std::error_code& ec) {
  add_telnet_option(option + "=" + value, ec);
}

void easy::set_telnet_options(std::shared_ptr<string_list> telnet_options) {
  std::error_code ec;
  set_telnet_options(std::move(telnet_options), ec);
  throw_error(ec, "set_telnet_options");
}

void easy::set_telnet_options(std::shared_ptr<string_list> telnet_options,
                              std::error_code& ec) {
  telnet_options_ = std::move(telnet_options);

  if (telnet_options_) {
    ec = std::error_code(
        native::curl_easy_setopt(handle_, native::CURLOPT_TELNETOPTIONS,
                                 telnet_options_->native_handle()));
  } else {
    ec = std::error_code(
        native::curl_easy_setopt(handle_, native::CURLOPT_TELNETOPTIONS, NULL));
  }
}

bool easy::has_post_data() const { return !post_fields_.empty() || form_; }

void easy::handle_completion(const std::error_code& err) {
  LOG_TRACE() << "easy::handle_completion easy="
              << reinterpret_cast<long>(this);

  timings_.mark_complete();
  if (sink_) {
    sink_->flush();
  }

  multi_registered_ = false;

  auto handler = std::function<void(const std::error_code& err)>(
      [](const std::error_code&) {});
  swap(handler, handler_);

  /* It's OK to call handler in libev thread context as it is limited to
   * Request::on_retry and Request::on_completed. All user code is executed in
   * coro context.
   */
  handler(err);
}

void easy::init() {
  initref_ = initialization::ensure_initialization();
  handle_ = native::curl_easy_init();

  if (!handle_) {
    throw std::bad_alloc();
  }

  set_private(this);
}

native::curl_socket_t easy::open_tcp_socket(native::curl_sockaddr* address) {
  std::error_code ec;
  std::unique_ptr<socket_type> socket(
      new socket_type(multi_->GetThreadControl()));

  LOG_TRACE() << "open_tcp_socket family=" << address->family;

  switch (address->family) {
    case AF_INET:
      socket->Open(socket_type::Domain::kIPv4);
      break;

    case AF_INET6:
      socket->Open(socket_type::Domain::kIPv6);
      break;

    default:
      return CURL_SOCKET_BAD;
  }

  if (ec) {
    return CURL_SOCKET_BAD;
  } else {
    auto si = std::make_shared<socket_info>(this, std::move(socket));
    multi_->socket_register(si);
    return si->socket->native_handle();
  }
}

size_t easy::write_function(char* ptr, size_t size, size_t nmemb,
                            void* userdata) {
  easy* self = static_cast<easy*>(userdata);
  size_t actual_size = size * nmemb;

  if (!actual_size) {
    return 0;
  }

  if (!self->sink_->write(ptr, actual_size)) {
    return 0;
  } else {
    return actual_size;
  }
}

size_t easy::read_function(void* ptr, size_t size, size_t nmemb,
                           void* userdata) {
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

int easy::seek_function(void* instream, native::curl_off_t offset, int origin) {
  // TODO we could allow the user to define an offset which this library should
  // consider as position zero for uploading chunks of the file
  // alternatively do tellg() on the source stream when it is first passed to
  // use_source() and use that as origin

  easy* self = static_cast<easy*>(instream);

  std::ios::seekdir dir;

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
                            native::curl_off_t ulnow) {
  easy* self = static_cast<easy*>(clientp);
  return self->progress_callback_(dltotal, dlnow, ultotal, ulnow) ? 0 : 1;
}

native::curl_socket_t easy::opensocket(void* clientp,
                                       native::curlsocktype purpose,
                                       struct native::curl_sockaddr* address) {
  easy* self = static_cast<easy*>(clientp);
  multi* multi_handle = self->multi_;
  native::curl_socket_t s = -1;

  // NOLINTNEXTLINE(hicpp-multiway-paths-covered)
  switch (purpose) {
    case native::CURLSOCKTYPE_IPCXN:
      switch (address->socktype) {
        case SOCK_STREAM:
          // Note to self: Why is address->protocol always set to zero?
          s = self->open_tcp_socket(address);
          if (s != -1 && multi_handle)
            multi_handle->Statistics().mark_open_socket();
          return s;

        case SOCK_DGRAM:
          // TODO implement - I've seen other libcurl wrappers with UDP
          // implementation, but have yet to read up on what this is used for
          return CURL_SOCKET_BAD;

        default:
          // unknown or invalid socket type
          return CURL_SOCKET_BAD;
      }
      break;

#ifdef CURLSOCKTYPE_ACCEPT
    case native::CURLSOCKTYPE_ACCEPT:
      // TODO implement - is this used for active FTP?
      return CURL_SOCKET_BAD;
#endif

    default:
      // unknown or invalid purpose
      return CURL_SOCKET_BAD;
  }
}

int easy::closesocket(void* clientp, native::curl_socket_t item) {
  auto* multi_handle = static_cast<multi*>(clientp);
  multi_handle->socket_cleanup(item);
  multi_handle->Statistics().mark_close_socket();
  return 0;
}
