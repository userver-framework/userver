/** @file curl-ev/easy.hpp
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        C++ wrapper for libcurl's easy interface
*/

#pragma once

#include <cstdio>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <curl-ev/error_code.hpp>
#include <curl-ev/form.hpp>
#include <curl-ev/ratelimit.hpp>
#include <curl-ev/url.hpp>

#include <userver/clients/http/local_stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {

class ThreadControl;

}  // namespace engine::ev

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPLEMENT_CURL_OPTION(FUNCTION_NAME, OPTION_NAME, OPTION_TYPE) \
  inline void FUNCTION_NAME(OPTION_TYPE arg) {                         \
    std::error_code ec;                                                \
    FUNCTION_NAME(arg, ec);                                            \
    throw_error(ec, PP_STRINGIZE(FUNCTION_NAME));                      \
  }                                                                    \
  inline void FUNCTION_NAME(OPTION_TYPE arg, std::error_code& ec) {    \
    ec = std::error_code(static_cast<errc::EasyErrorCode>(             \
        native::curl_easy_setopt(handle_, OPTION_NAME, arg)));         \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPLEMENT_CURL_OPTION_BOOLEAN(FUNCTION_NAME, OPTION_NAME)            \
  inline void FUNCTION_NAME(bool enabled) {                                  \
    std::error_code ec;                                                      \
    FUNCTION_NAME(enabled, ec);                                              \
    throw_error(ec, PP_STRINGIZE(FUNCTION_NAME));                            \
  }                                                                          \
  inline void FUNCTION_NAME(bool enabled, std::error_code& ec) {             \
    ec = std::error_code(static_cast<errc::EasyErrorCode>(                   \
        native::curl_easy_setopt(handle_, OPTION_NAME, enabled ? 1L : 0L))); \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPLEMENT_CURL_OPTION_ENUM(FUNCTION_NAME, OPTION_NAME, ENUM_TYPE, \
                                   OPTION_TYPE)                           \
  inline void FUNCTION_NAME(ENUM_TYPE arg) {                              \
    std::error_code ec;                                                   \
    FUNCTION_NAME(arg, ec);                                               \
    throw_error(ec, PP_STRINGIZE(FUNCTION_NAME));                         \
  }                                                                       \
  inline void FUNCTION_NAME(ENUM_TYPE arg, std::error_code& ec) {         \
    ec = std::error_code(                                                 \
        static_cast<errc::EasyErrorCode>(native::curl_easy_setopt(        \
            handle_, OPTION_NAME, static_cast<OPTION_TYPE>(arg))));       \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPLEMENT_CURL_OPTION_STRING(FUNCTION_NAME, OPTION_NAME)           \
  inline void FUNCTION_NAME(const char* str) {                             \
    std::error_code ec;                                                    \
    FUNCTION_NAME(str, ec);                                                \
    throw_error(ec, PP_STRINGIZE(FUNCTION_NAME));                          \
  }                                                                        \
  inline void FUNCTION_NAME(const char* str, std::error_code& ec) {        \
    ec = std::error_code(static_cast<errc::EasyErrorCode>(                 \
        native::curl_easy_setopt(handle_, OPTION_NAME, str)));             \
  }                                                                        \
  inline void FUNCTION_NAME(const std::string& str) {                      \
    std::error_code ec;                                                    \
    FUNCTION_NAME(str, ec);                                                \
    throw_error(ec, PP_STRINGIZE(FUNCTION_NAME));                          \
  }                                                                        \
  inline void FUNCTION_NAME(const std::string& str, std::error_code& ec) { \
    ec = std::error_code(static_cast<errc::EasyErrorCode>(                 \
        native::curl_easy_setopt(handle_, OPTION_NAME, str.c_str())));     \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPLEMENT_CURL_OPTION_GET_STRING_VIEW(FUNCTION_NAME, OPTION_NAME) \
  inline std::string_view FUNCTION_NAME() {                               \
    std::error_code ec;                                                   \
    auto info = FUNCTION_NAME(ec);                                        \
    throw_error(ec, PP_STRINGIZE(FUNCTION_NAME));                         \
    return info;                                                          \
  }                                                                       \
  inline std::string_view FUNCTION_NAME(std::error_code& ec) {            \
    char* info = nullptr;                                                 \
    ec = std::error_code(static_cast<errc::EasyErrorCode>(                \
        native::curl_easy_getinfo(handle_, OPTION_NAME, &info)));         \
    return info ? info : std::string_view{};                              \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPLEMENT_CURL_OPTION_GET_LONG(FUNCTION_NAME, OPTION_NAME)         \
  inline long FUNCTION_NAME() {                                            \
    long info;                                                             \
    std::error_code ec = std::error_code(static_cast<errc::EasyErrorCode>( \
        native::curl_easy_getinfo(handle_, OPTION_NAME, &info)));          \
    throw_error(ec, PP_STRINGIZE(FUNCTION_NAME));                          \
    return info;                                                           \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPLEMENT_CURL_OPTION_GET_CURL_OFF_T(FUNCTION_NAME, OPTION_NAME)   \
  inline long FUNCTION_NAME() {                                            \
    native::curl_off_t info;                                               \
    std::error_code ec = std::error_code(static_cast<errc::EasyErrorCode>( \
        native::curl_easy_getinfo(handle_, OPTION_NAME, &info)));          \
    throw_error(ec, PP_STRINGIZE(FUNCTION_NAME));                          \
    return info;                                                           \
  }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define IMPLEMENT_CURL_OPTION_GET_LIST(FUNCTION_NAME, OPTION_NAME)         \
  inline std::vector<std::string> FUNCTION_NAME() {                        \
    struct native::curl_slist* info;                                       \
    std::vector<std::string> results;                                      \
    std::error_code ec = std::error_code(static_cast<errc::EasyErrorCode>( \
        native::curl_easy_getinfo(handle_, OPTION_NAME, &info)));          \
    throw_error(ec, PP_STRINGIZE(FUNCTION_NAME));                          \
    struct native::curl_slist* it = info;                                  \
    while (it) {                                                           \
      results.emplace_back(it->data);                                      \
      it = it->next;                                                       \
    }                                                                      \
    native::curl_slist_free_all(info);                                     \
    return results;                                                        \
  }

namespace curl {
// class form;
class multi;
class share;
class string_list;

class easy final : public std::enable_shared_from_this<easy> {
 public:
  using handler_type = std::function<void(std::error_code err)>;
  using time_point = std::chrono::steady_clock::time_point;

  static easy* from_native(native::CURL* native_easy);

  // Creates an initialized but unbound easy, use GetBound() for usable
  // instances. May block on resolver initialization.
  static std::shared_ptr<const easy> CreateBlocking();

  easy(native::CURL*, multi*);
  easy(const easy&) = delete;
  ~easy();

  // Makes a clone of an initialized easy, hopefully non-blocking (skips full
  // resolver initialization).
  std::shared_ptr<easy> GetBoundBlocking(multi&) const;

  const multi* GetMulti() const { return multi_; }

  inline native::CURL* native_handle() { return handle_; }
  engine::ev::ThreadControl& GetThreadControl();

  void perform();
  void perform(std::error_code& ec);
  void async_perform(handler_type handler);
  void cancel();
  void reset();
  void set_source(std::shared_ptr<std::istream> source);
  void set_source(std::shared_ptr<std::istream> source, std::error_code& ec);
  void set_sink(std::string* sink);
  void set_sink(std::string* sink, std::error_code& ec);

  using progress_callback_t =
      std::function<bool(native::curl_off_t dltotal, native::curl_off_t dlnow,
                         native::curl_off_t ultotal, native::curl_off_t ulnow)>;
  void unset_progress_callback();
  void set_progress_callback(progress_callback_t progress_callback);

  // behavior options

  IMPLEMENT_CURL_OPTION_BOOLEAN(set_verbose, native::CURLOPT_VERBOSE);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_header, native::CURLOPT_HEADER);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_no_progress, native::CURLOPT_NOPROGRESS);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_no_signal, native::CURLOPT_NOSIGNAL);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_wildcard_match,
                                native::CURLOPT_WILDCARDMATCH);

  // callback options

  using write_function_t = size_t (*)(char* ptr, size_t size, size_t nmemb,
                                      void* userdata);
  IMPLEMENT_CURL_OPTION(set_write_function, native::CURLOPT_WRITEFUNCTION,
                        write_function_t);
  IMPLEMENT_CURL_OPTION(set_write_data, native::CURLOPT_WRITEDATA, void*);
  using read_function_t = size_t (*)(void* ptr, size_t size, size_t nmemb,
                                     void* userdata);
  IMPLEMENT_CURL_OPTION(set_read_function, native::CURLOPT_READFUNCTION,
                        read_function_t);
  IMPLEMENT_CURL_OPTION(set_read_data, native::CURLOPT_READDATA, void*);
  using ioctl_function_t = native::curlioerr (*)(native::CURL* handle, int cmd,
                                                 void* clientp);
  IMPLEMENT_CURL_OPTION(set_ioctl_function, native::CURLOPT_IOCTLFUNCTION,
                        ioctl_function_t);
  IMPLEMENT_CURL_OPTION(set_ioctl_data, native::CURLOPT_IOCTLDATA, void*);
  using seek_function_t = int (*)(void* instream, native::curl_off_t offset,
                                  int origin);
  IMPLEMENT_CURL_OPTION(set_seek_function, native::CURLOPT_SEEKFUNCTION,
                        seek_function_t);
  IMPLEMENT_CURL_OPTION(set_seek_data, native::CURLOPT_SEEKDATA, void*);
  using sockopt_function_t = int (*)(void* clientp,
                                     native::curl_socket_t curlfd,
                                     native::curlsocktype purpose);
  IMPLEMENT_CURL_OPTION(set_sockopt_function, native::CURLOPT_SOCKOPTFUNCTION,
                        sockopt_function_t);
  IMPLEMENT_CURL_OPTION(set_sockopt_data, native::CURLOPT_SOCKOPTDATA, void*);
  using opensocket_function_t =
      native::curl_socket_t (*)(void* clientp, native::curlsocktype purpose,
                                struct native::curl_sockaddr* address);
  IMPLEMENT_CURL_OPTION(set_opensocket_function,
                        native::CURLOPT_OPENSOCKETFUNCTION,
                        opensocket_function_t);
  IMPLEMENT_CURL_OPTION(set_opensocket_data, native::CURLOPT_OPENSOCKETDATA,
                        void*);
  using closesocket_function_t = int (*)(void* clientp,
                                         native::curl_socket_t item);
  IMPLEMENT_CURL_OPTION(set_closesocket_function,
                        native::CURLOPT_CLOSESOCKETFUNCTION,
                        closesocket_function_t);
  IMPLEMENT_CURL_OPTION(set_closesocket_data, native::CURLOPT_CLOSESOCKETDATA,
                        void*);
  using progress_function_t = int (*)(void* clientp, double dltotal,
                                      double dlnow, double ultotal,
                                      double ulnow);
  IMPLEMENT_CURL_OPTION(set_progress_function, native::CURLOPT_PROGRESSFUNCTION,
                        progress_function_t);
  IMPLEMENT_CURL_OPTION(set_progress_data, native::CURLOPT_PROGRESSDATA, void*);
  using xferinfo_function_t = int (*)(void* clientp, native::curl_off_t dltotal,
                                      native::curl_off_t dlnow,
                                      native::curl_off_t ultotal,
                                      native::curl_off_t ulnow);
  IMPLEMENT_CURL_OPTION(set_xferinfo_function, native::CURLOPT_XFERINFOFUNCTION,
                        xferinfo_function_t);
  IMPLEMENT_CURL_OPTION(set_xferinfo_data, native::CURLOPT_XFERINFODATA, void*);
  using header_function_t = size_t (*)(void* ptr, size_t size, size_t nmemb,
                                       void* userdata);
  IMPLEMENT_CURL_OPTION(set_header_function, native::CURLOPT_HEADERFUNCTION,
                        header_function_t);
  IMPLEMENT_CURL_OPTION(set_header_data, native::CURLOPT_HEADERDATA, void*);
  using debug_callback_t = int (*)(native::CURL*, native::curl_infotype, char*,
                                   size_t, void*);
  IMPLEMENT_CURL_OPTION(set_debug_callback, native::CURLOPT_DEBUGFUNCTION,
                        debug_callback_t);
  IMPLEMENT_CURL_OPTION(set_debug_data, native::CURLOPT_DEBUGDATA, void*);
  using ssl_ctx_function_t = native::CURLcode (*)(native::CURL* curl,
                                                  void* sslctx, void* parm);
  IMPLEMENT_CURL_OPTION(set_ssl_ctx_function, native::CURLOPT_SSL_CTX_FUNCTION,
                        ssl_ctx_function_t);
  IMPLEMENT_CURL_OPTION(set_ssl_ctx_data, native::CURLOPT_SSL_CTX_DATA, void*);
  using interleave_function_t = size_t (*)(void* ptr, size_t size, size_t nmemb,
                                           void* userdata);
  IMPLEMENT_CURL_OPTION(set_interleave_function,
                        native::CURLOPT_INTERLEAVEFUNCTION,
                        interleave_function_t);
  IMPLEMENT_CURL_OPTION(set_interleave_data, native::CURLOPT_INTERLEAVEDATA,
                        void*);

  // error options

  IMPLEMENT_CURL_OPTION(set_error_buffer, native::CURLOPT_ERRORBUFFER, char*);
  IMPLEMENT_CURL_OPTION(set_stderr, native::CURLOPT_STDERR, FILE*);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_fail_on_error, native::CURLOPT_FAILONERROR);

  // network options

  void set_url(std::string url_str);
  void set_url(std::string url_str, std::error_code& ec);
  const std::string& get_original_url() const;
  const url& get_easy_url() const;

  IMPLEMENT_CURL_OPTION(set_protocols, native::CURLOPT_PROTOCOLS, long);
  IMPLEMENT_CURL_OPTION(set_redir_protocols, native::CURLOPT_REDIR_PROTOCOLS,
                        long);
  IMPLEMENT_CURL_OPTION_STRING(set_proxy, native::CURLOPT_PROXY);
  IMPLEMENT_CURL_OPTION(set_proxy_port, native::CURLOPT_PROXYPORT, long);
  IMPLEMENT_CURL_OPTION(set_proxy_type, native::CURLOPT_PROXYTYPE, long);
  IMPLEMENT_CURL_OPTION_STRING(set_no_proxy, native::CURLOPT_NOPROXY);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_http_proxy_tunnel,
                                native::CURLOPT_HTTPPROXYTUNNEL);
  IMPLEMENT_CURL_OPTION_STRING(set_socks5_gsapi_service,
                               native::CURLOPT_SOCKS5_GSSAPI_SERVICE);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_socks5_gsapi_nec,
                                native::CURLOPT_SOCKS5_GSSAPI_NEC);
  IMPLEMENT_CURL_OPTION_STRING(set_interface, native::CURLOPT_INTERFACE);
  IMPLEMENT_CURL_OPTION(set_local_port, native::CURLOPT_LOCALPORT, long);
  IMPLEMENT_CURL_OPTION(set_local_port_range, native::CURLOPT_LOCALPORTRANGE,
                        long);
  IMPLEMENT_CURL_OPTION(set_dns_cache_timeout,
                        native::CURLOPT_DNS_CACHE_TIMEOUT, long);
  IMPLEMENT_CURL_OPTION(set_buffer_size, native::CURLOPT_BUFFERSIZE, long);
  IMPLEMENT_CURL_OPTION(set_port, native::CURLOPT_PORT, long);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_tcp_no_delay, native::CURLOPT_TCP_NODELAY);
  IMPLEMENT_CURL_OPTION(set_address_scope, native::CURLOPT_ADDRESS_SCOPE, long);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_tcp_keep_alive,
                                native::CURLOPT_TCP_KEEPALIVE);
  IMPLEMENT_CURL_OPTION(set_tcp_keep_idle, native::CURLOPT_TCP_KEEPIDLE, long);
  IMPLEMENT_CURL_OPTION(set_tcp_keep_intvl, native::CURLOPT_TCP_KEEPINTVL,
                        long);
  IMPLEMENT_CURL_OPTION_STRING(set_unix_socket_path,
                               native::CURLOPT_UNIX_SOCKET_PATH);
  IMPLEMENT_CURL_OPTION(set_connect_to, native::CURLOPT_CONNECT_TO,
                        native::curl_slist*);
  // authentication options

  enum netrc_t {
    netrc_optional = native::CURL_NETRC_OPTIONAL,
    netrc_ignored = native::CURL_NETRC_IGNORED,
    netrc_required = native::CURL_NETRC_REQUIRED
  };
  IMPLEMENT_CURL_OPTION_ENUM(set_netrc, native::CURLOPT_NETRC, netrc_t, long);
  IMPLEMENT_CURL_OPTION_STRING(set_netrc_file, native::CURLOPT_NETRC_FILE);
  IMPLEMENT_CURL_OPTION_STRING(set_user, native::CURLOPT_USERNAME);
  IMPLEMENT_CURL_OPTION_STRING(set_password, native::CURLOPT_PASSWORD);
  IMPLEMENT_CURL_OPTION_STRING(set_proxy_user, native::CURLOPT_PROXYUSERNAME);
  IMPLEMENT_CURL_OPTION_STRING(set_proxy_password,
                               native::CURLOPT_PROXYPASSWORD);
  enum httpauth_t {
    auth_basic = CURLAUTH_BASIC,
    auth_digest,
    auth_digest_ie,
    auth_gss_negotiate,
    auth_ntml,
    auth_nhtml_wb,
    auth_any,
    auth_any_safe
  };
  inline void set_http_auth(httpauth_t auth, bool auth_only) {
    std::error_code ec;
    set_http_auth(auth, auth_only, ec);
    throw_error(ec, "set_http_auth failed");
  }
  inline void set_http_auth(httpauth_t auth, bool auth_only,
                            std::error_code& ec) {
    auto l = static_cast<long>(auth | (auth_only ? CURLAUTH_ONLY : 0UL));
    ec = std::error_code(static_cast<errc::EasyErrorCode>(
        native::curl_easy_setopt(handle_, native::CURLOPT_HTTPAUTH, l)));
  }
  IMPLEMENT_CURL_OPTION(set_tls_auth_type, native::CURLOPT_TLSAUTH_TYPE, long);
  IMPLEMENT_CURL_OPTION_STRING(set_tls_auth_user,
                               native::CURLOPT_TLSAUTH_USERNAME);
  IMPLEMENT_CURL_OPTION_STRING(set_tls_auth_password,
                               native::CURLOPT_TLSAUTH_PASSWORD);
  enum proxyauth_t {
    proxy_auth_basic = CURLAUTH_BASIC,
    proxy_auth_digest = CURLAUTH_DIGEST,
    proxy_auth_digest_ie = CURLAUTH_DIGEST_IE,
    proxy_auth_bearer = CURLAUTH_BEARER,
    proxy_auth_negotiate = CURLAUTH_NEGOTIATE,
    proxy_auth_ntlm = CURLAUTH_NTLM,
    proxy_auth_ntlm_wb = CURLAUTH_NTLM_WB,
    proxy_auth_any = CURLAUTH_ANY,
    proxy_auth_anysafe = CURLAUTH_ANYSAFE
  };
  IMPLEMENT_CURL_OPTION(set_proxy_auth, native::CURLOPT_PROXYAUTH, long);

  // HTTP options

  IMPLEMENT_CURL_OPTION_BOOLEAN(set_auto_referrer, native::CURLOPT_AUTOREFERER);
  IMPLEMENT_CURL_OPTION_STRING(set_accept_encoding,
                               native::CURLOPT_ACCEPT_ENCODING);
  IMPLEMENT_CURL_OPTION_STRING(set_transfer_encoding,
                               native::CURLOPT_TRANSFER_ENCODING);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_follow_location,
                                native::CURLOPT_FOLLOWLOCATION);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_unrestricted_auth,
                                native::CURLOPT_UNRESTRICTED_AUTH);
  IMPLEMENT_CURL_OPTION(set_max_redirs, native::CURLOPT_MAXREDIRS, long);
  IMPLEMENT_CURL_OPTION(set_post_redir, native::CURLOPT_POSTREDIR, long);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_post, native::CURLOPT_POST);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_put, native::CURLOPT_PUT);
  void set_post_fields(std::string&& post_fields);
  void set_post_fields(std::string&& post_fields, std::error_code& ec);
  IMPLEMENT_CURL_OPTION(set_post_fields, native::CURLOPT_POSTFIELDS, void*);
  IMPLEMENT_CURL_OPTION(set_post_field_size, native::CURLOPT_POSTFIELDSIZE,
                        long);
  IMPLEMENT_CURL_OPTION(set_post_field_size_large,
                        native::CURLOPT_POSTFIELDSIZE_LARGE,
                        native::curl_off_t);

  void set_http_post(std::shared_ptr<form> form);
  void set_http_post(std::shared_ptr<form> form, std::error_code& ec);

  IMPLEMENT_CURL_OPTION_STRING(set_referer, native::CURLOPT_REFERER);
  IMPLEMENT_CURL_OPTION_STRING(set_user_agent, native::CURLOPT_USERAGENT);
  enum class EmptyHeaderAction { kSend, kDoNotSend };
  enum class DuplicateHeaderAction { kAdd, kSkip, kReplace };
  void add_header(
      std::string_view name, std::string_view value,
      EmptyHeaderAction empty_header_action = EmptyHeaderAction::kSend,
      DuplicateHeaderAction duplicate_header_action =
          DuplicateHeaderAction::kAdd);
  void add_header(
      std::string_view name, std::string_view value, std::error_code& ec,
      EmptyHeaderAction empty_header_action = EmptyHeaderAction::kSend,
      DuplicateHeaderAction duplicate_header_action =
          DuplicateHeaderAction::kAdd);
  void add_header(std::string_view name, std::string_view value,
                  DuplicateHeaderAction duplicate_header_action);
  void add_header(std::string_view name, std::string_view value,
                  std::error_code& ec,
                  DuplicateHeaderAction duplicate_header_action);
  void add_header(const char* header);
  void add_header(const char* header, std::error_code& ec);
  void add_header(const std::string& header);
  void add_header(const std::string& header, std::error_code& ec);
  void set_headers(std::shared_ptr<string_list> headers);
  void set_headers(std::shared_ptr<string_list> headers, std::error_code& ec);
  std::optional<std::string_view> FindHeaderByName(std::string_view name) const;
  void add_proxy_header(
      std::string_view name, std::string_view value,
      EmptyHeaderAction empty_header_action = EmptyHeaderAction::kSend,
      DuplicateHeaderAction duplicate_header_action =
          DuplicateHeaderAction::kAdd);
  void add_proxy_header(
      std::string_view name, std::string_view value, std::error_code& ec,
      EmptyHeaderAction empty_header_action = EmptyHeaderAction::kSend,
      DuplicateHeaderAction duplicate_header_action =
          DuplicateHeaderAction::kAdd);
  void add_proxy_header(const char* header, std::error_code& ec);
  void add_http200_alias(const std::string& http200_alias);
  void add_http200_alias(const std::string& http200_alias, std::error_code& ec);
  void set_http200_aliases(std::shared_ptr<string_list> http200_aliases);
  void set_http200_aliases(std::shared_ptr<string_list> http200_aliases,
                           std::error_code& ec);
  IMPLEMENT_CURL_OPTION_STRING(set_cookie, native::CURLOPT_COOKIE);
  IMPLEMENT_CURL_OPTION_STRING(set_cookie_file, native::CURLOPT_COOKIEFILE);
  IMPLEMENT_CURL_OPTION_STRING(set_cookie_jar, native::CURLOPT_COOKIEJAR);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_cookie_session,
                                native::CURLOPT_COOKIESESSION);
  IMPLEMENT_CURL_OPTION_STRING(set_cookie_list, native::CURLOPT_COOKIELIST);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_http_get, native::CURLOPT_HTTPGET);
  enum http_version_t {
    http_version_none = native::CURL_HTTP_VERSION_NONE,
    http_version_1_0 = native::CURL_HTTP_VERSION_1_0,
    http_version_1_1 = native::CURL_HTTP_VERSION_1_1,
    http_version_2_0 = native::CURL_HTTP_VERSION_2_0,
    http_vertion_2tls = native::CURL_HTTP_VERSION_2TLS,
    http_version_2_prior_knowledge =
        native::CURL_HTTP_VERSION_2_PRIOR_KNOWLEDGE,
  };
  IMPLEMENT_CURL_OPTION_ENUM(set_http_version, native::CURLOPT_HTTP_VERSION,
                             http_version_t, long);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_ignore_content_length,
                                native::CURLOPT_IGNORE_CONTENT_LENGTH);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_http_content_decoding,
                                native::CURLOPT_HTTP_CONTENT_DECODING);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_http_transfer_decoding,
                                native::CURLOPT_HTTP_TRANSFER_DECODING);

  // protocol options

  IMPLEMENT_CURL_OPTION_BOOLEAN(set_transfer_text,
                                native::CURLOPT_TRANSFERTEXT);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_transfer_mode,
                                native::CURLOPT_PROXY_TRANSFER_MODE);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_crlf, native::CURLOPT_CRLF);
  IMPLEMENT_CURL_OPTION_STRING(set_range, native::CURLOPT_RANGE);
  IMPLEMENT_CURL_OPTION(set_resume_from, native::CURLOPT_RESUME_FROM, long);
  IMPLEMENT_CURL_OPTION(set_resume_from_large,
                        native::CURLOPT_RESUME_FROM_LARGE, native::curl_off_t);
  IMPLEMENT_CURL_OPTION_STRING(set_custom_request,
                               native::CURLOPT_CUSTOMREQUEST);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_file_time, native::CURLOPT_FILETIME);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_no_body, native::CURLOPT_NOBODY);
  IMPLEMENT_CURL_OPTION(set_in_file_size, native::CURLOPT_INFILESIZE, long);
  IMPLEMENT_CURL_OPTION(set_in_file_size_large,
                        native::CURLOPT_INFILESIZE_LARGE, native::curl_off_t);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_upload, native::CURLOPT_UPLOAD);
  IMPLEMENT_CURL_OPTION(set_max_file_size, native::CURLOPT_MAXFILESIZE, long);
  IMPLEMENT_CURL_OPTION(set_max_file_size_large,
                        native::CURLOPT_MAXFILESIZE_LARGE, native::curl_off_t);
  enum time_condition_t {
    if_modified_since = native::CURL_TIMECOND_IFMODSINCE,
    if_unmodified_since = native::CURL_TIMECOND_IFUNMODSINCE
  };
  IMPLEMENT_CURL_OPTION_ENUM(set_time_condition, native::CURLOPT_TIMECONDITION,
                             time_condition_t, long);
  IMPLEMENT_CURL_OPTION(set_time_value, native::CURLOPT_TIMEVALUE, long);

  // connection options

  IMPLEMENT_CURL_OPTION(set_timeout, native::CURLOPT_TIMEOUT, long);
  IMPLEMENT_CURL_OPTION(set_timeout_ms, native::CURLOPT_TIMEOUT_MS, long);
  IMPLEMENT_CURL_OPTION(set_low_speed_limit, native::CURLOPT_LOW_SPEED_LIMIT,
                        long);
  IMPLEMENT_CURL_OPTION(set_low_speed_time, native::CURLOPT_LOW_SPEED_TIME,
                        long);
  IMPLEMENT_CURL_OPTION(set_max_send_speed_large,
                        native::CURLOPT_MAX_SEND_SPEED_LARGE,
                        native::curl_off_t);
  IMPLEMENT_CURL_OPTION(set_max_recv_speed_large,
                        native::CURLOPT_MAX_RECV_SPEED_LARGE,
                        native::curl_off_t);
  IMPLEMENT_CURL_OPTION(set_max_connects, native::CURLOPT_MAXCONNECTS, long);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_fresh_connect,
                                native::CURLOPT_FRESH_CONNECT);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_forbot_reuse, native::CURLOPT_FORBID_REUSE);
  IMPLEMENT_CURL_OPTION(set_connect_timeout, native::CURLOPT_CONNECTTIMEOUT,
                        long);
  IMPLEMENT_CURL_OPTION(set_connect_timeout_ms,
                        native::CURLOPT_CONNECTTIMEOUT_MS, long);
  enum ip_resolve_t {
    ip_resolve_whatever = CURL_IPRESOLVE_WHATEVER,
    ip_resolve_v4 = CURL_IPRESOLVE_V4,
    ip_resolve_v6 = CURL_IPRESOLVE_V6
  };
  IMPLEMENT_CURL_OPTION_ENUM(set_ip_resolve, native::CURLOPT_IPRESOLVE,
                             ip_resolve_t, long);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_connect_only, native::CURLOPT_CONNECT_ONLY);
  enum use_ssl_t {
    use_ssl_none = native::CURLUSESSL_NONE,
    use_ssl_try = native::CURLUSESSL_TRY,
    use_ssl_control = native::CURLUSESSL_CONTROL,
    use_ssl_all = native::CURLUSESSL_ALL
  };
  IMPLEMENT_CURL_OPTION_ENUM(set_use_ssl, native::CURLOPT_USE_SSL, use_ssl_t,
                             long);
  void add_resolve(const std::string& host, const std::string& port,
                   const std::string& addr);
  void add_resolve(const std::string& host, const std::string& port,
                   const std::string& addr, std::error_code& ec);
  void set_resolves(std::shared_ptr<string_list> resolved_hosts);
  void set_resolves(std::shared_ptr<string_list> resolved_hosts,
                    std::error_code& ec);
  IMPLEMENT_CURL_OPTION_STRING(set_dns_servers, native::CURLOPT_DNS_SERVERS);
  IMPLEMENT_CURL_OPTION(set_accept_timeout_ms, native::CURLOPT_ACCEPTTIMEOUT_MS,
                        long);

  // SSL and security options

  IMPLEMENT_CURL_OPTION_STRING(set_ssl_cert, native::CURLOPT_SSLCERT);
  IMPLEMENT_CURL_OPTION_STRING(set_ssl_cert_type, native::CURLOPT_SSLCERTTYPE);
  IMPLEMENT_CURL_OPTION_STRING(set_ssl_key, native::CURLOPT_SSLKEY);
  IMPLEMENT_CURL_OPTION_STRING(set_ssl_key_type, native::CURLOPT_SSLKEYTYPE);
  IMPLEMENT_CURL_OPTION_STRING(set_ssl_key_passwd, native::CURLOPT_KEYPASSWD);
  IMPLEMENT_CURL_OPTION_STRING(set_ssl_engine, native::CURLOPT_SSLENGINE);
  IMPLEMENT_CURL_OPTION_STRING(set_ssl_engine_default,
                               native::CURLOPT_SSLENGINE_DEFAULT);
  enum ssl_version_t {
    ssl_version_default = native::CURL_SSLVERSION_DEFAULT,
    ssl_version_tls_v1 = native::CURL_SSLVERSION_TLSv1,
    ssl_version_ssl_v2 = native::CURL_SSLVERSION_SSLv2,
    ssl_version_ssl_v3 = native::CURL_SSLVERSION_SSLv3
  };
  IMPLEMENT_CURL_OPTION_ENUM(set_ssl_version, native::CURLOPT_SSLVERSION,
                             ssl_version_t, long);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_ssl_verify_peer,
                                native::CURLOPT_SSL_VERIFYPEER);
  IMPLEMENT_CURL_OPTION_STRING(set_ca_info, native::CURLOPT_CAINFO);
  IMPLEMENT_CURL_OPTION_STRING(set_issuer_cert, native::CURLOPT_ISSUERCERT);
  IMPLEMENT_CURL_OPTION_STRING(set_ca_file, native::CURLOPT_CAPATH);
  IMPLEMENT_CURL_OPTION_STRING(set_crl_file, native::CURLOPT_CRLFILE);
  inline void set_ssl_verify_host(bool verify_host) {
    std::error_code ec;
    set_ssl_verify_host(verify_host, ec);
    throw_error(ec, "set_ssl_verify_host failed");
  }
  inline void set_ssl_verify_host(bool verify_host, std::error_code& ec) {
    ec = std::error_code{
        static_cast<errc::EasyErrorCode>(native::curl_easy_setopt(
            handle_, native::CURLOPT_SSL_VERIFYHOST, verify_host ? 2L : 0L))};
  }
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_cert_info, native::CURLOPT_CERTINFO);
  IMPLEMENT_CURL_OPTION_STRING(set_random_file, native::CURLOPT_RANDOM_FILE);
  IMPLEMENT_CURL_OPTION_STRING(set_egd_socket, native::CURLOPT_EGDSOCKET);
  IMPLEMENT_CURL_OPTION_STRING(set_ssl_cipher_list,
                               native::CURLOPT_SSL_CIPHER_LIST);
  IMPLEMENT_CURL_OPTION_BOOLEAN(set_ssl_session_id_cache,
                                native::CURLOPT_SSL_SESSIONID_CACHE);
  IMPLEMENT_CURL_OPTION(set_ssl_options, native::CURLOPT_SSL_OPTIONS, long);
  IMPLEMENT_CURL_OPTION_STRING(set_krb_level, native::CURLOPT_KRBLEVEL);
  IMPLEMENT_CURL_OPTION(set_gssapi_delegation,
                        native::CURLOPT_GSSAPI_DELEGATION, long);

  // SSH options

  IMPLEMENT_CURL_OPTION(set_ssh_auth_types, native::CURLOPT_SSH_AUTH_TYPES,
                        long);
  IMPLEMENT_CURL_OPTION_STRING(set_ssh_host_public_key_md5,
                               native::CURLOPT_SSH_HOST_PUBLIC_KEY_MD5);
  IMPLEMENT_CURL_OPTION_STRING(set_ssh_public_key_file,
                               native::CURLOPT_SSH_PUBLIC_KEYFILE);
  IMPLEMENT_CURL_OPTION_STRING(set_ssh_private_key_file,
                               native::CURLOPT_SSH_PRIVATE_KEYFILE);
  IMPLEMENT_CURL_OPTION_STRING(set_ssh_known_hosts,
                               native::CURLOPT_SSH_KNOWNHOSTS);
  IMPLEMENT_CURL_OPTION(set_ssh_key_function, native::CURLOPT_SSH_KEYFUNCTION,
                        void*);  // TODO curl_sshkeycallback?
  IMPLEMENT_CURL_OPTION(set_ssh_key_data, native::CURLOPT_SSH_KEYDATA, void*);

  // other options

  IMPLEMENT_CURL_OPTION(set_private, native::CURLOPT_PRIVATE, void*);
  void set_share(std::shared_ptr<share> share);
  void set_share(std::shared_ptr<share> share, std::error_code& ec);
  IMPLEMENT_CURL_OPTION(set_new_file_perms, native::CURLOPT_NEW_FILE_PERMS,
                        long);
  IMPLEMENT_CURL_OPTION(set_new_directory_perms,
                        native::CURLOPT_NEW_DIRECTORY_PERMS, long);

  // getters

  IMPLEMENT_CURL_OPTION_GET_STRING_VIEW(get_effective_url,
                                        native::CURLINFO_EFFECTIVE_URL);
  IMPLEMENT_CURL_OPTION_GET_LONG(get_response_code,
                                 native::CURLINFO_RESPONSE_CODE);
  // CURLINFO_TOTAL_TIME
  // CURLINFO_NAMELOOKUP_TIME
  // CURLINFO_CONNECT_TIME
  // CURLINFO_PRETRANSFER_TIME
  // CURLINFO_SIZE_UPLOAD
  IMPLEMENT_CURL_OPTION_GET_CURL_OFF_T(get_size_upload,
                                       native::CURLINFO_SIZE_UPLOAD_T);
  // CURLINFO_SIZE_DOWNLOAD
  IMPLEMENT_CURL_OPTION_GET_CURL_OFF_T(get_size_download,
                                       native::CURLINFO_SIZE_DOWNLOAD_T);
  // CURLINFO_SPEED_DOWNLOAD
  IMPLEMENT_CURL_OPTION_GET_CURL_OFF_T(get_speed_download_bps,
                                       native::CURLINFO_SPEED_DOWNLOAD_T);
  // CURLINFO_SPEED_UPLOAD
  IMPLEMENT_CURL_OPTION_GET_CURL_OFF_T(get_speed_upload_bps,
                                       native::CURLINFO_SPEED_UPLOAD_T);
  IMPLEMENT_CURL_OPTION_GET_LONG(get_header_size, native::CURLINFO_HEADER_SIZE);
  IMPLEMENT_CURL_OPTION_GET_LONG(get_request_size,
                                 native::CURLINFO_REQUEST_SIZE);
  IMPLEMENT_CURL_OPTION_GET_LONG(get_ssl_verifyresult,
                                 native::CURLINFO_SSL_VERIFYRESULT);
  // CURLINFO_FILETIME
  IMPLEMENT_CURL_OPTION_GET_CURL_OFF_T(get_filetime_sec,
                                       native::CURLINFO_FILETIME_T);
  // CURLINFO_CONTENT_LENGTH_DOWNLOAD
  IMPLEMENT_CURL_OPTION_GET_CURL_OFF_T(
      get_content_length_download, native::CURLINFO_CONTENT_LENGTH_DOWNLOAD_T);
  // CURLINFO_CONTENT_LENGTH_UPLOAD
  IMPLEMENT_CURL_OPTION_GET_CURL_OFF_T(
      get_content_length_upload, native::CURLINFO_CONTENT_LENGTH_UPLOAD_T);
  // CURLINFO_STARTTRANSFER_TIME
  IMPLEMENT_CURL_OPTION_GET_STRING_VIEW(get_content_type,
                                        native::CURLINFO_CONTENT_TYPE);
  // CURLINFO_REDIRECT_TIME
  IMPLEMENT_CURL_OPTION_GET_LONG(get_redirect_count,
                                 native::CURLINFO_REDIRECT_COUNT);
  // CURLINFO_PRIVATE
  IMPLEMENT_CURL_OPTION_GET_LONG(get_http_connectcode,
                                 native::CURLINFO_HTTP_CONNECTCODE);
  IMPLEMENT_CURL_OPTION_GET_LONG(get_httpauth_avail,
                                 native::CURLINFO_HTTPAUTH_AVAIL);
  IMPLEMENT_CURL_OPTION_GET_LONG(get_proxyauth_avail,
                                 native::CURLINFO_PROXYAUTH_AVAIL);
  IMPLEMENT_CURL_OPTION_GET_LONG(get_os_errno, native::CURLINFO_OS_ERRNO);
  IMPLEMENT_CURL_OPTION_GET_LONG(get_num_connects,
                                 native::CURLINFO_NUM_CONNECTS);
  IMPLEMENT_CURL_OPTION_GET_LIST(get_ssl_engines, native::CURLINFO_SSL_ENGINES);
  IMPLEMENT_CURL_OPTION_GET_LIST(get_cookielist, native::CURLINFO_COOKIELIST);
  IMPLEMENT_CURL_OPTION_GET_LONG(get_lastsocket, native::CURLINFO_LASTSOCKET);
  IMPLEMENT_CURL_OPTION_GET_STRING_VIEW(get_ftp_entry_path,
                                        native::CURLINFO_FTP_ENTRY_PATH);
  IMPLEMENT_CURL_OPTION_GET_STRING_VIEW(get_redirect_url,
                                        native::CURLINFO_REDIRECT_URL);
  IMPLEMENT_CURL_OPTION_GET_STRING_VIEW(get_primary_ip,
                                        native::CURLINFO_PRIMARY_IP);
  // CURLINFO_APPCONNECT_TIME
  // CURLINFO_CERTINFO
  IMPLEMENT_CURL_OPTION_GET_LONG(get_condition_unmet,
                                 native::CURLINFO_CONDITION_UNMET);
  IMPLEMENT_CURL_OPTION_GET_STRING_VIEW(get_rtsp_session_id,
                                        native::CURLINFO_RTSP_SESSION_ID);
  IMPLEMENT_CURL_OPTION_GET_LONG(get_rtsp_client_cseq,
                                 native::CURLINFO_RTSP_CLIENT_CSEQ);
  IMPLEMENT_CURL_OPTION_GET_LONG(get_rtsp_server_cseq,
                                 native::CURLINFO_RTSP_SERVER_CSEQ);
  IMPLEMENT_CURL_OPTION_GET_LONG(get_rtsp_cseq_recv,
                                 native::CURLINFO_RTSP_CSEQ_RECV);
  IMPLEMENT_CURL_OPTION_GET_LONG(get_primary_port,
                                 native::CURLINFO_PRIMARY_PORT);
  IMPLEMENT_CURL_OPTION_GET_STRING_VIEW(get_local_ip,
                                        native::CURLINFO_LOCAL_IP);
  IMPLEMENT_CURL_OPTION_GET_LONG(get_local_port, native::CURLINFO_LOCAL_PORT);
  // CURLINFO_TLS_SESSION
  // CURLINFO_ACTIVESOCKET
  // CURLINFO_TLS_SSL_PTR
  IMPLEMENT_CURL_OPTION_GET_LONG(get_http_version,
                                 native::CURLINFO_HTTP_VERSION);
  IMPLEMENT_CURL_OPTION_GET_LONG(get_proxy_ssl_verifyresult,
                                 native::CURLINFO_PROXY_SSL_VERIFYRESULT);
  IMPLEMENT_CURL_OPTION_GET_LONG(get_protocol, native::CURLINFO_PROTOCOL);
  IMPLEMENT_CURL_OPTION_GET_STRING_VIEW(get_scheme, native::CURLINFO_SCHEME);
  IMPLEMENT_CURL_OPTION_GET_CURL_OFF_T(get_total_time_usec,
                                       native::CURLINFO_TOTAL_TIME_T);
  IMPLEMENT_CURL_OPTION_GET_CURL_OFF_T(get_namelookup_time_usec,
                                       native::CURLINFO_NAMELOOKUP_TIME_T);
  IMPLEMENT_CURL_OPTION_GET_CURL_OFF_T(get_connect_time_usec,
                                       native::CURLINFO_CONNECT_TIME_T);
  IMPLEMENT_CURL_OPTION_GET_CURL_OFF_T(get_pretransfer_time_usec,
                                       native::CURLINFO_PRETRANSFER_TIME_T);
  IMPLEMENT_CURL_OPTION_GET_CURL_OFF_T(get_starttransfer_time_usec,
                                       native::CURLINFO_STARTTRANSFER_TIME_T);
  IMPLEMENT_CURL_OPTION_GET_CURL_OFF_T(get_redirect_time_usec,
                                       native::CURLINFO_REDIRECT_TIME_T);
  IMPLEMENT_CURL_OPTION_GET_CURL_OFF_T(get_appconnect_time_usec,
                                       native::CURLINFO_APPCONNECT_TIME_T);
  IMPLEMENT_CURL_OPTION_GET_CURL_OFF_T(get_retry_after_sec,
                                       native::CURLINFO_RETRY_AFTER);

  bool has_post_data() const;

  const std::string& get_post_data() const;

  std::string extract_post_data();

  inline bool operator<(const easy& other) const { return (this < &other); }

  void handle_completion(const std::error_code& err);

  void mark_retry();

  clients::http::LocalStats get_local_stats();

  std::error_code rate_limit_error() const;

  time_point::duration time_to_start() const;

 private:
  static size_t header_function(void* ptr, size_t size, size_t nmemb,
                                void* userdata);

  native::curl_socket_t open_tcp_socket(native::curl_sockaddr* address);
  void cancel(size_t request_num);

  static size_t write_function(char* ptr, size_t size, size_t nmemb,
                               void* userdata) noexcept;
  static size_t read_function(void* ptr, size_t size, size_t nmemb,
                              void* userdata) noexcept;
  static int seek_function(void* instream, native::curl_off_t offset,
                           int origin) noexcept;
  static int xferinfo_function(void* clientp, native::curl_off_t dltotal,
                               native::curl_off_t dlnow,
                               native::curl_off_t ultotal,
                               native::curl_off_t ulnow) noexcept;
  static native::curl_socket_t opensocket(
      void* clientp, native::curlsocktype purpose,
      struct native::curl_sockaddr* address) noexcept;
  static int closesocket(void* clientp, native::curl_socket_t item) noexcept;

  // do_ev_* methods run in libev thread
  void do_ev_async_perform(handler_type handler, size_t request_num);
  void do_ev_cancel(size_t request_num);

  void mark_start_performing();
  void mark_open_socket();

  native::CURL* handle_{nullptr};
  multi* multi_;
  size_t request_counter_{0};
  size_t cancelled_request_max_{0};
  bool multi_registered_{false};
  std::string orig_url_str_;
  url url_;
  handler_type handler_;
  std::shared_ptr<std::istream> source_;
  std::string* sink_{nullptr};
  std::string post_fields_;
  std::shared_ptr<form> form_;
  std::shared_ptr<string_list> headers_;
  std::shared_ptr<string_list> proxy_headers_;
  std::shared_ptr<string_list> http200_aliases_;
  std::shared_ptr<string_list> resolved_hosts_;
  std::shared_ptr<share> share_;
  progress_callback_t progress_callback_;
  std::size_t retries_count_{0};
  std::size_t sockets_opened_{0};
  std::error_code rate_limit_error_;

  time_point start_performing_ts_{};
  const time_point construct_ts_;
};
}  // namespace curl

#undef IMPLEMENT_CURL_OPTION
#undef IMPLEMENT_CURL_OPTION_BOOLEAN
#undef IMPLEMENT_CURL_OPTION_ENUM
#undef IMPLEMENT_CURL_OPTION_STRING
#undef IMPLEMENT_CURL_OPTION_GET_STRING_VIEW
#undef IMPLEMENT_CURL_OPTION_GET_LONG
#undef IMPLEMENT_CURL_OPTION_GET_LIST

USERVER_NAMESPACE_END
