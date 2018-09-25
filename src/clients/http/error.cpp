#include <clients/http/error.hpp>

#include <curl-ev/error_code.hpp>

namespace clients {
namespace http {

BaseCodeException::BaseCodeException(std::error_code ec, const std::string& msg)
    : BaseException(msg + ", curl error: " + ec.message()), ec_(ec) {}

std::exception_ptr PrepareException(std::error_code ec) {
  if (ec.category() != curl::errc::get_easy_category())
    return std::make_exception_ptr(BaseCodeException(ec, "Unknown exception"));

  switch (ec.value()) {
    case curl::errc::easy::could_not_resovle_host:
      return std::make_exception_ptr(DNSProblemException(ec, "DNS problem"));

    case curl::errc::easy::operation_timedout:
      return std::make_exception_ptr(TimeoutException("Timeout happened"));

    case curl::errc::easy::ssl_connect_error:
    case curl::errc::easy::peer_failed_verification:
    case curl::errc::easy::ssl_cipher:
    case curl::errc::easy::ssl_certproblem:
    case curl::errc::easy::ssl_cacert:
    case curl::errc::easy::ssl_cacert_badfile:
    case curl::errc::easy::ssl_issuer_error:
    case curl::errc::easy::ssl_crl_badfile:
      return std::make_exception_ptr(SSLException(ec, "SSL problem"));

    case curl::errc::easy::bad_function_argument:
      return std::make_exception_ptr(
          BadArgumentException(ec, "Incorrect argument"));

    case curl::errc::easy::too_many_redirects:
      return std::make_exception_ptr(
          TooManyRedirectsException(ec, "Too many redirects"));

    case curl::errc::easy::send_error:
    case curl::errc::easy::recv_error:
    case curl::errc::easy::could_not_connect:
      return std::make_exception_ptr(
          NetworkProblemException(ec, "Network problems"));

    case curl::errc::easy::login_denied:
      return std::make_exception_ptr(
          AuthFailedException(ec, "Failed to login"));

    default:
      return std::make_exception_ptr(TechnicalError(ec, "Technical error"));
  }
}

}  // namespace http
}  // namespace clients
