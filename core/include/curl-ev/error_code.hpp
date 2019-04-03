/** @file curl-ev/error_code.hpp
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        Integration of libcurl's error codes (CURLcode) into boost.system's
   error_code class
*/

#pragma once

#include <system_error>

#include "config.hpp"
#include "native.hpp"

namespace curl {
namespace errc {
namespace easy {
enum easy_error_codes {
  success = native::CURLE_OK,
  unsupported_protocol = native::CURLE_UNSUPPORTED_PROTOCOL,
  failed_init = native::CURLE_FAILED_INIT,
  url_malformat = native::CURLE_URL_MALFORMAT,
  not_built_in = native::CURLE_NOT_BUILT_IN,
  could_not_resolve_proxy = native::CURLE_COULDNT_RESOLVE_PROXY,
  could_not_resovle_host = native::CURLE_COULDNT_RESOLVE_HOST,
  could_not_connect = native::CURLE_COULDNT_CONNECT,
  ftp_weird_server_reply = native::CURLE_FTP_WEIRD_SERVER_REPLY,
  remote_access_denied = native::CURLE_REMOTE_ACCESS_DENIED,
  ftp_accept_failed = native::CURLE_FTP_ACCEPT_FAILED,
  ftp_weird_pass_reply = native::CURLE_FTP_WEIRD_PASS_REPLY,
  ftp_accept_timeout = native::CURLE_FTP_ACCEPT_TIMEOUT,
  ftp_weird_pasv_reply = native::CURLE_FTP_WEIRD_PASV_REPLY,
  ftp_weird_227_format = native::CURLE_FTP_WEIRD_227_FORMAT,
  ftp_cant_get_host = native::CURLE_FTP_CANT_GET_HOST,
  ftp_couldnt_set_type = native::CURLE_FTP_COULDNT_SET_TYPE,
  partial_file = native::CURLE_PARTIAL_FILE,
  ftp_couldnt_retr_file = native::CURLE_FTP_COULDNT_RETR_FILE,
  quote_error = native::CURLE_QUOTE_ERROR,
  http_returned_error = native::CURLE_HTTP_RETURNED_ERROR,
  write_error = native::CURLE_WRITE_ERROR,
  upload_failed = native::CURLE_UPLOAD_FAILED,
  read_error = native::CURLE_READ_ERROR,
  out_of_memory = native::CURLE_OUT_OF_MEMORY,
  operation_timedout = native::CURLE_OPERATION_TIMEDOUT,
  ftp_port_failed = native::CURLE_FTP_PORT_FAILED,
  ftp_couldnt_use_rest = native::CURLE_FTP_COULDNT_USE_REST,
  range_error = native::CURLE_RANGE_ERROR,
  http_post_error = native::CURLE_HTTP_POST_ERROR,
  ssl_connect_error = native::CURLE_SSL_CONNECT_ERROR,
  bad_download_resume = native::CURLE_BAD_DOWNLOAD_RESUME,
  file_couldnt_read_file = native::CURLE_FILE_COULDNT_READ_FILE,
  ldap_cannot_bind = native::CURLE_LDAP_CANNOT_BIND,
  ldap_search_failed = native::CURLE_LDAP_SEARCH_FAILED,
  function_not_found = native::CURLE_FUNCTION_NOT_FOUND,
  aborted_by_callback = native::CURLE_ABORTED_BY_CALLBACK,
  bad_function_argument = native::CURLE_BAD_FUNCTION_ARGUMENT,
  interface_failed = native::CURLE_INTERFACE_FAILED,
  too_many_redirects = native::CURLE_TOO_MANY_REDIRECTS,
  unknown_option = native::CURLE_UNKNOWN_OPTION,
  telnet_option_syntax = native::CURLE_TELNET_OPTION_SYNTAX,
  peer_failed_verification = native::CURLE_PEER_FAILED_VERIFICATION,
  got_nothing = native::CURLE_GOT_NOTHING,
  ssl_engine_notfound = native::CURLE_SSL_ENGINE_NOTFOUND,
  ssl_engine_setfailed = native::CURLE_SSL_ENGINE_SETFAILED,
  send_error = native::CURLE_SEND_ERROR,
  recv_error = native::CURLE_RECV_ERROR,
  ssl_certproblem = native::CURLE_SSL_CERTPROBLEM,
  ssl_cipher = native::CURLE_SSL_CIPHER,
  ssl_cacert = native::CURLE_SSL_CACERT,
  bad_content_encoding = native::CURLE_BAD_CONTENT_ENCODING,
  ldap_invalid_url = native::CURLE_LDAP_INVALID_URL,
  filesize_exceeded = native::CURLE_FILESIZE_EXCEEDED,
  use_ssl_failed = native::CURLE_USE_SSL_FAILED,
  send_fail_rewind = native::CURLE_SEND_FAIL_REWIND,
  ssl_engine_initfailed = native::CURLE_SSL_ENGINE_INITFAILED,
  login_denied = native::CURLE_LOGIN_DENIED,
  tftp_notfound = native::CURLE_TFTP_NOTFOUND,
  tftp_perm = native::CURLE_TFTP_PERM,
  remote_disk_full = native::CURLE_REMOTE_DISK_FULL,
  tftp_illegal = native::CURLE_TFTP_ILLEGAL,
  tftp_unknownid = native::CURLE_TFTP_UNKNOWNID,
  remote_file_exists = native::CURLE_REMOTE_FILE_EXISTS,
  tftp_nosuchuser = native::CURLE_TFTP_NOSUCHUSER,
  conv_failed = native::CURLE_CONV_FAILED,
  conv_reqd = native::CURLE_CONV_REQD,
  ssl_cacert_badfile = native::CURLE_SSL_CACERT_BADFILE,
  remote_file_not_found = native::CURLE_REMOTE_FILE_NOT_FOUND,
  ssh = native::CURLE_SSH,
  ssl_shutdown_failed = native::CURLE_SSL_SHUTDOWN_FAILED,
  again = native::CURLE_AGAIN,
  ssl_crl_badfile = native::CURLE_SSL_CRL_BADFILE,
  ssl_issuer_error = native::CURLE_SSL_ISSUER_ERROR,
  ftp_pret_failed = native::CURLE_FTP_PRET_FAILED,
  rtsp_cseq_error = native::CURLE_RTSP_CSEQ_ERROR,
  rtsp_session_error = native::CURLE_RTSP_SESSION_ERROR,
  ftp_bad_file_list = native::CURLE_FTP_BAD_FILE_LIST,
  chunk_failed = native::CURLE_CHUNK_FAILED
};
}  // namespace easy

namespace multi {
enum multi_error_codes {
  success = native::CURLM_OK,
  bad_handle = native::CURLM_BAD_HANDLE,
  bad_easy_handle = native::CURLM_BAD_EASY_HANDLE,
  out_of_memory = native::CURLM_OUT_OF_MEMORY,
  intenral_error = native::CURLM_INTERNAL_ERROR,
  bad_socket = native::CURLM_BAD_SOCKET,
  unknown_option = native::CURLM_UNKNOWN_OPTION
};
}  // namespace multi

namespace share {
enum share_error_codes {
  success = native::CURLSHE_OK,
  bad_option = native::CURLSHE_BAD_OPTION,
  in_use = native::CURLSHE_IN_USE,
  invalid = native::CURLSHE_INVALID,
  nomem = native::CURLSHE_NOMEM,
  not_built_in = native::CURLSHE_NOT_BUILT_IN
};
}  // namespace share

namespace form {
enum form_error_codes {
  success = native::CURL_FORMADD_OK,
  memory = native::CURL_FORMADD_MEMORY,
  option_twice = native::CURL_FORMADD_OPTION_TWICE,
  null = native::CURL_FORMADD_NULL,
  unknown_option = native::CURL_FORMADD_UNKNOWN_OPTION,
  incomplete = native::CURL_FORMADD_INCOMPLETE,
  illegal_array = native::CURL_FORMADD_ILLEGAL_ARRAY,
  disabled = native::CURL_FORMADD_DISABLED
};
}  // namespace form

const std::error_category& get_easy_category() noexcept;
const std::error_category& get_multi_category() noexcept;
const std::error_category& get_share_category() noexcept;
const std::error_category& get_form_category() noexcept;
}  // namespace errc
}  // namespace curl

namespace std {
template <>
struct is_error_code_enum<curl::errc::easy::easy_error_codes> {
  static const bool value = true;
};

template <>
struct is_error_code_enum<curl::native::CURLcode> {
  static const bool value = true;
};

template <>
struct is_error_code_enum<curl::errc::multi::multi_error_codes> {
  static const bool value = true;
};

template <>
struct is_error_code_enum<curl::native::CURLMcode> {
  static const bool value = true;
};

template <>
struct is_error_code_enum<curl::errc::share::share_error_codes> {
  static const bool value = true;
};

template <>
struct is_error_code_enum<curl::native::CURLSHcode> {
  static const bool value = true;
};

template <>
struct is_error_code_enum<curl::errc::form::form_error_codes> {
  static const bool value = true;
};

template <>
struct is_error_code_enum<curl::native::CURLFORMcode> {
  static const bool value = true;
};
}  // namespace std

namespace curl {
namespace errc {
namespace easy {
inline std::error_code make_error_code(easy_error_codes e) {
  return std::error_code(static_cast<int>(e), get_easy_category());
}
}  // namespace easy

namespace multi {
inline std::error_code make_error_code(multi_error_codes e) {
  return std::error_code(static_cast<int>(e), get_multi_category());
}
}  // namespace multi

namespace share {
inline std::error_code make_error_code(share_error_codes e) {
  return std::error_code(static_cast<int>(e), get_share_category());
}
}  // namespace share

namespace form {
inline std::error_code make_error_code(form_error_codes e) {
  return std::error_code(static_cast<int>(e), get_form_category());
}
}  // namespace form
}  // namespace errc

namespace native {
inline std::error_code make_error_code(CURLcode e) {
  return make_error_code(static_cast<errc::easy::easy_error_codes>(e));
}

inline std::error_code make_error_code(CURLMcode e) {
  return make_error_code(static_cast<errc::multi::multi_error_codes>(e));
}

inline std::error_code make_error_code(CURLSHcode e) {
  return make_error_code(static_cast<errc::share::share_error_codes>(e));
}

inline std::error_code make_error_code(CURLFORMcode e) {
  return make_error_code(static_cast<errc::form::form_error_codes>(e));
}
}  // namespace native
}  // namespace curl
