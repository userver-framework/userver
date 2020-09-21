#include <clients/http/error.hpp>

#include <fmt/format.h>

#include <curl-ev/error_code.hpp>

namespace clients::http {

BaseCodeException::BaseCodeException(std::error_code ec,
                                     std::string_view message,
                                     std::string_view url,
                                     const LocalStats& stats)
    : BaseException(fmt::format("{}, curl error: {}, url: {}", message,
                                ec.message(), url),
                    stats),
      ec_(ec) {}

HttpException::HttpException(int code, const LocalStats& stats)
    : HttpException(code, stats, {}) {}

HttpException::HttpException(int code, const LocalStats& stats,
                             std::string_view message)
    : BaseException(fmt::format("Raise for status exception, code = {}{}{}",
                                code, message.empty() ? "" : ": ", message),
                    stats),
      code_(code) {}

std::exception_ptr PrepareException(std::error_code ec, const std::string& url,
                                    const LocalStats& stats) {
  if (ec.category() != curl::errc::get_easy_category())
    return std::make_exception_ptr(
        BaseCodeException(ec, "Unknown exception", url, stats));

  switch (ec.value()) {
    case curl::errc::easy::could_not_resovle_host:
      return std::make_exception_ptr(
          DNSProblemException(ec, "DNS problem", url, stats));

    case curl::errc::easy::operation_timedout:
      return std::make_exception_ptr(TimeoutException(
          fmt::format("Timeout happened, url: {}", url), stats));

    case curl::errc::easy::ssl_connect_error:
    case curl::errc::easy::peer_failed_verification:
    case curl::errc::easy::ssl_cipher:
    case curl::errc::easy::ssl_certproblem:
#if CURLE_SSL_CACERT != CURLE_PEER_FAILED_VERIFICATION
    case curl::errc::easy::ssl_cacert:
#endif
    case curl::errc::easy::ssl_cacert_badfile:
    case curl::errc::easy::ssl_issuer_error:
    case curl::errc::easy::ssl_crl_badfile:
      return std::make_exception_ptr(
          SSLException(ec, "SSL problem", url, stats));

    case curl::errc::easy::bad_function_argument:
      return std::make_exception_ptr(
          BadArgumentException(ec, "Incorrect argument", url, stats));

    case curl::errc::easy::too_many_redirects:
      return std::make_exception_ptr(
          TooManyRedirectsException(ec, "Too many redirects", url, stats));

    case curl::errc::easy::send_error:
    case curl::errc::easy::recv_error:
    case curl::errc::easy::could_not_connect:
      return std::make_exception_ptr(
          NetworkProblemException(ec, "Network problem", url, stats));

    case curl::errc::easy::login_denied:
      return std::make_exception_ptr(
          AuthFailedException(ec, "Failed to login", url, stats));

    default:
      return std::make_exception_ptr(
          TechnicalError(ec, "Technical error", url, stats));
  }
}

}  // namespace clients::http
