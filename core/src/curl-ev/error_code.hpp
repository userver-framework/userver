/** @file curl-ev/error_code.hpp
        curl-ev: wrapper for integrating libcurl with libev applications
        Copyright (c) 2013 Oliver Kuckertz <oliver.kuckertz@mologie.de>
        See COPYING for license information.

        Integration of libcurl's error codes (CURLcode) into boost.system's
   error_code class
*/

#pragma once

#include <system_error>

#include <curl-ev/native.hpp>

USERVER_NAMESPACE_BEGIN

namespace curl::errc {

enum class EasyErrorCode {
  kSuccess = native::CURLE_OK,
  kUnsupportedProtocol = native::CURLE_UNSUPPORTED_PROTOCOL,
  kFailedInit = native::CURLE_FAILED_INIT,
  kUrlMalformat = native::CURLE_URL_MALFORMAT,
  kNotBuiltIn = native::CURLE_NOT_BUILT_IN,
  kCouldNotResolveProxy = native::CURLE_COULDNT_RESOLVE_PROXY,
  kCouldNotResolveHost = native::CURLE_COULDNT_RESOLVE_HOST,
  kCouldNotConnect = native::CURLE_COULDNT_CONNECT,
  kFtpWeirdServerReply = native::CURLE_FTP_WEIRD_SERVER_REPLY,
  kRemoteAccessDenied = native::CURLE_REMOTE_ACCESS_DENIED,
  kFtpAcceptFailed = native::CURLE_FTP_ACCEPT_FAILED,
  kFtpWeirdPassReply = native::CURLE_FTP_WEIRD_PASS_REPLY,
  kFtpAcceptTimeout = native::CURLE_FTP_ACCEPT_TIMEOUT,
  kFtpWeirdPasvReply = native::CURLE_FTP_WEIRD_PASV_REPLY,
  kFtpWeird_227_format = native::CURLE_FTP_WEIRD_227_FORMAT,
  kFtpCantGetHost = native::CURLE_FTP_CANT_GET_HOST,
  kHttp2 = native::CURLE_HTTP2,
  kFtpCouldntSetType = native::CURLE_FTP_COULDNT_SET_TYPE,
  kPartialFile = native::CURLE_PARTIAL_FILE,
  kFtpCouldntRetrFile = native::CURLE_FTP_COULDNT_RETR_FILE,
  kQuoteError = native::CURLE_QUOTE_ERROR,
  kHttpReturnedError = native::CURLE_HTTP_RETURNED_ERROR,
  kWriteError = native::CURLE_WRITE_ERROR,
  kUploadFailed = native::CURLE_UPLOAD_FAILED,
  kReadError = native::CURLE_READ_ERROR,
  kOutOfMemory = native::CURLE_OUT_OF_MEMORY,
  kOperationTimedout = native::CURLE_OPERATION_TIMEDOUT,
  kFtpPortFailed = native::CURLE_FTP_PORT_FAILED,
  kFtpCouldntUseRest = native::CURLE_FTP_COULDNT_USE_REST,
  kRangeError = native::CURLE_RANGE_ERROR,
  kHttpPostError = native::CURLE_HTTP_POST_ERROR,
  kSslConnectError = native::CURLE_SSL_CONNECT_ERROR,
  kBadDownloadResume = native::CURLE_BAD_DOWNLOAD_RESUME,
  kFileCouldntReadFile = native::CURLE_FILE_COULDNT_READ_FILE,
  kLdapCannotBind = native::CURLE_LDAP_CANNOT_BIND,
  kLdapSearchFailed = native::CURLE_LDAP_SEARCH_FAILED,
  kFunctionNotFound = native::CURLE_FUNCTION_NOT_FOUND,
  kAbortedByCallback = native::CURLE_ABORTED_BY_CALLBACK,
  kBadFunctionArgument = native::CURLE_BAD_FUNCTION_ARGUMENT,
  kInterfaceFailed = native::CURLE_INTERFACE_FAILED,
  kTooManyRedirects = native::CURLE_TOO_MANY_REDIRECTS,
  kUnknownOption = native::CURLE_UNKNOWN_OPTION,
  kTelnetOptionSyntax = native::CURLE_TELNET_OPTION_SYNTAX,
  kGotNothing = native::CURLE_GOT_NOTHING,
  kSslEngineNotfound = native::CURLE_SSL_ENGINE_NOTFOUND,
  kSslEngineSetfailed = native::CURLE_SSL_ENGINE_SETFAILED,
  kSendError = native::CURLE_SEND_ERROR,
  kRecvError = native::CURLE_RECV_ERROR,
  kSslCertproblem = native::CURLE_SSL_CERTPROBLEM,
  kSslCipher = native::CURLE_SSL_CIPHER,
  kPeerFailedVerification = native::CURLE_PEER_FAILED_VERIFICATION,
  kBadContentEncoding = native::CURLE_BAD_CONTENT_ENCODING,
  kLdapInvalidUrl = native::CURLE_LDAP_INVALID_URL,
  kFilesizeExceeded = native::CURLE_FILESIZE_EXCEEDED,
  kUseSslFailed = native::CURLE_USE_SSL_FAILED,
  kSendFailRewind = native::CURLE_SEND_FAIL_REWIND,
  kSslEngineInitfailed = native::CURLE_SSL_ENGINE_INITFAILED,
  kLoginDenied = native::CURLE_LOGIN_DENIED,
  kTftpNotfound = native::CURLE_TFTP_NOTFOUND,
  kTftpPerm = native::CURLE_TFTP_PERM,
  kRemoteDiskFull = native::CURLE_REMOTE_DISK_FULL,
  kTftpIllegal = native::CURLE_TFTP_ILLEGAL,
  kTftpUnknownid = native::CURLE_TFTP_UNKNOWNID,
  kRemoteFileExists = native::CURLE_REMOTE_FILE_EXISTS,
  kTftpNosuchuser = native::CURLE_TFTP_NOSUCHUSER,
  kConvFailed = native::CURLE_CONV_FAILED,
  kConvReqd = native::CURLE_CONV_REQD,
  kSslCacertBadfile = native::CURLE_SSL_CACERT_BADFILE,
  kRemoteFileNotFound = native::CURLE_REMOTE_FILE_NOT_FOUND,
  kSsh = native::CURLE_SSH,
  kSslShutdownFailed = native::CURLE_SSL_SHUTDOWN_FAILED,
  kAgain = native::CURLE_AGAIN,
  kSslCrlBadfile = native::CURLE_SSL_CRL_BADFILE,
  kSslIssuerError = native::CURLE_SSL_ISSUER_ERROR,
  kFtpPretFailed = native::CURLE_FTP_PRET_FAILED,
  kRtspCseqError = native::CURLE_RTSP_CSEQ_ERROR,
  kRtspSessionError = native::CURLE_RTSP_SESSION_ERROR,
  kFtpBadFileList = native::CURLE_FTP_BAD_FILE_LIST,
  kChunkFailed = native::CURLE_CHUNK_FAILED,
  kNoConnectionAvailable = native::CURLE_NO_CONNECTION_AVAILABLE,
  kSslPinnedpubkeynotmatch = native::CURLE_SSL_PINNEDPUBKEYNOTMATCH,
  kSslInvalidcertstatus = native::CURLE_SSL_INVALIDCERTSTATUS,
  kHttp2_stream = native::CURLE_HTTP2_STREAM,
  kRecursiveApiCall = native::CURLE_RECURSIVE_API_CALL,
  kAuthError = native::CURLE_AUTH_ERROR,
  kHttp3 = native::CURLE_HTTP3,
};

enum class MultiErrorCode {
  kSuccess = native::CURLM_OK,
  kBadHandle = native::CURLM_BAD_HANDLE,
  kBadEasyHandle = native::CURLM_BAD_EASY_HANDLE,
  kOutOfMemory = native::CURLM_OUT_OF_MEMORY,
  kIntenralError = native::CURLM_INTERNAL_ERROR,
  kBadSocket = native::CURLM_BAD_SOCKET,
  kUnknownOption = native::CURLM_UNKNOWN_OPTION,
  kAddedAlready = native::CURLM_ADDED_ALREADY,
  kRecursiveApiCall = native::CURLM_RECURSIVE_API_CALL,
  kWakeupFailure = native::CURLM_WAKEUP_FAILURE,
};

enum class ShareErrorCode {
  kSuccess = native::CURLSHE_OK,
  kBadOption = native::CURLSHE_BAD_OPTION,
  kInUse = native::CURLSHE_IN_USE,
  kInvalid = native::CURLSHE_INVALID,
  kNomem = native::CURLSHE_NOMEM,
  kNotBuiltIn = native::CURLSHE_NOT_BUILT_IN
};

enum class FormErrorCode {
  kSuccess = native::CURL_FORMADD_OK,
  kMemory = native::CURL_FORMADD_MEMORY,
  kOptionTwice = native::CURL_FORMADD_OPTION_TWICE,
  kNull = native::CURL_FORMADD_NULL,
  kUnknownOption = native::CURL_FORMADD_UNKNOWN_OPTION,
  kIncomplete = native::CURL_FORMADD_INCOMPLETE,
  kIllegalArray = native::CURL_FORMADD_ILLEGAL_ARRAY,
  kDisabled = native::CURL_FORMADD_DISABLED
};

enum class UrlErrorCode {
  kSuccess = native::CURLUE_OK,
  kBadHandle = native::CURLUE_BAD_HANDLE,
  kBadPartpointer = native::CURLUE_BAD_PARTPOINTER,
  kMalformedInput = native::CURLUE_MALFORMED_INPUT,
  kBadPortNumber = native::CURLUE_BAD_PORT_NUMBER,
  kUnsupportedScheme = native::CURLUE_UNSUPPORTED_SCHEME,
  kUrldecode = native::CURLUE_URLDECODE,
  kOutOfMemory = native::CURLUE_OUT_OF_MEMORY,
  kUserNotAllowed = native::CURLUE_USER_NOT_ALLOWED,
  kUnknownPart = native::CURLUE_UNKNOWN_PART,
  kNoScheme = native::CURLUE_NO_SCHEME,
  kNoUser = native::CURLUE_NO_USER,
  kNoPassword = native::CURLUE_NO_PASSWORD,
  kNoOptions = native::CURLUE_NO_OPTIONS,
  kNoHost = native::CURLUE_NO_HOST,
  kNoPort = native::CURLUE_NO_PORT,
  kNoQuery = native::CURLUE_NO_QUERY,
  kNoFragment = native::CURLUE_NO_FRAGMENT,
};

enum class RateLimitErrorCode {
  kSuccess,
  kGlobalSocketLimit,
  kPerHostSocketLimit,
};

const std::error_category& GetEasyCategory() noexcept;
const std::error_category& GetMultiCategory() noexcept;
const std::error_category& GetShareCategory() noexcept;
const std::error_category& GetFormCategory() noexcept;
const std::error_category& GetUrlCategory() noexcept;
const std::error_category& GetRateLimitCategory() noexcept;

}  // namespace curl::errc

USERVER_NAMESPACE_END

namespace std {

template <>
struct is_error_code_enum<USERVER_NAMESPACE::curl::errc::EasyErrorCode>
    : std::true_type {};

template <>
struct is_error_code_enum<USERVER_NAMESPACE::curl::errc::MultiErrorCode>
    : std::true_type {};

template <>
struct is_error_code_enum<USERVER_NAMESPACE::curl::errc::ShareErrorCode>
    : std::true_type {};

template <>
struct is_error_code_enum<USERVER_NAMESPACE::curl::errc::FormErrorCode>
    : std::true_type {};

template <>
struct is_error_code_enum<USERVER_NAMESPACE::curl::errc::UrlErrorCode>
    : std::true_type {};

template <>
struct is_error_code_enum<USERVER_NAMESPACE::curl::errc::RateLimitErrorCode>
    : std::true_type {};

}  // namespace std

USERVER_NAMESPACE_BEGIN

namespace curl::errc {

inline std::error_code make_error_code(EasyErrorCode e) {
  return {static_cast<int>(e), GetEasyCategory()};
}

inline std::error_code make_error_code(MultiErrorCode e) {
  return {static_cast<int>(e), GetMultiCategory()};
}

inline std::error_code make_error_code(ShareErrorCode e) {
  return {static_cast<int>(e), GetShareCategory()};
}

inline std::error_code make_error_code(FormErrorCode e) {
  return {static_cast<int>(e), GetFormCategory()};
}

inline std::error_code make_error_code(UrlErrorCode e) {
  return {static_cast<int>(e), GetUrlCategory()};
}

inline std::error_code make_error_code(RateLimitErrorCode e) {
  return {static_cast<int>(e), GetRateLimitCategory()};
}

}  // namespace curl::errc

USERVER_NAMESPACE_END
