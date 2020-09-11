#include <clients/http/error.hpp>

#include <curl-ev/error_code.hpp>
#include <curl-ev/local_stats.hpp>

namespace clients {
namespace http {

BaseCodeException::BaseCodeException(std::error_code ec, const std::string& msg,
                                     const curl::LocalStats& stats)
    : BaseException(msg + ", curl error: " + ec.message(), stats), ec_(ec) {}

std::exception_ptr PrepareException(std::error_code ec, const std::string& url,
                                    const curl::LocalStats& stats) {
  if (ec.category() != curl::errc::get_easy_category())
    return std::make_exception_ptr(
        BaseCodeException(ec, "Unknown exception (" + url + ")", stats));

  switch (ec.value()) {
    case curl::errc::easy::could_not_resovle_host:
      return std::make_exception_ptr(
          DNSProblemException(ec, "DNS problem", stats));

    case curl::errc::easy::operation_timedout:
      return std::make_exception_ptr(
          TimeoutException("Timeout happened", stats));

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
      return std::make_exception_ptr(SSLException(ec, "SSL problem", stats));

    case curl::errc::easy::bad_function_argument:
      return std::make_exception_ptr(
          BadArgumentException(ec, "Incorrect argument", stats));

    case curl::errc::easy::too_many_redirects:
      return std::make_exception_ptr(
          TooManyRedirectsException(ec, "Too many redirects", stats));

    case curl::errc::easy::send_error:
    case curl::errc::easy::recv_error:
    case curl::errc::easy::could_not_connect:
      return std::make_exception_ptr(
          NetworkProblemException(ec, "Network problems", stats));

    case curl::errc::easy::login_denied:
      return std::make_exception_ptr(
          AuthFailedException(ec, "Failed to login", stats));

    default:
      return std::make_exception_ptr(
          TechnicalError(ec, "Technical error", stats));
  }
}

}  // namespace http
}  // namespace clients
