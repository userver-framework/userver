#include <userver/clients/http/error.hpp>

#include <fmt/format.h>

#include <curl-ev/error_code.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
namespace {

std::exception_ptr PrepareEasyException(std::error_code ec,
                                        std::string_view url,
                                        const LocalStats& stats) {
  using ErrorCode = curl::errc::EasyErrorCode;

  UASSERT(ec.category() == curl::errc::GetEasyCategory());

  switch (static_cast<ErrorCode>(ec.value())) {
    case ErrorCode::kCouldNotResolveHost:
      return std::make_exception_ptr(
          DNSProblemException(ec, "DNS problem", url, stats));

    case ErrorCode::kOperationTimedout:
      return std::make_exception_ptr(TimeoutException(
          fmt::format("Timeout happened, url: {}", url), stats));

    case ErrorCode::kSslConnectError:
    case ErrorCode::kPeerFailedVerification:
    case ErrorCode::kSslCipher:
    case ErrorCode::kSslCertproblem:
    case ErrorCode::kSslCacertBadfile:
    case ErrorCode::kSslIssuerError:
    case ErrorCode::kSslCrlBadfile:
      return std::make_exception_ptr(
          SSLException(ec, "SSL problem", url, stats));

    case ErrorCode::kBadFunctionArgument:
      return std::make_exception_ptr(
          BadArgumentException(ec, "Incorrect argument", url, stats));

    case ErrorCode::kTooManyRedirects:
      return std::make_exception_ptr(
          TooManyRedirectsException(ec, "Too many redirects", url, stats));

    case ErrorCode::kSendError:
    case ErrorCode::kRecvError:
    case ErrorCode::kCouldNotConnect:
      return std::make_exception_ptr(
          NetworkProblemException(ec, "Network problem", url, stats));

    case ErrorCode::kLoginDenied:
      return std::make_exception_ptr(
          AuthFailedException(ec, "Failed to login", url, stats));

    default:
      return std::make_exception_ptr(
          TechnicalError(ec, "Technical error", url, stats));
  }
}

std::exception_ptr PrepareRateLimitException(std::error_code ec,
                                             std::string_view url,
                                             const LocalStats& stats) {
  UASSERT(ec.category() == curl::errc::GetRateLimitCategory());

  return std::make_exception_ptr(
      NetworkProblemException(ec, "Rate limited", url, stats));
}

}  // namespace

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

std::exception_ptr PrepareException(std::error_code ec, std::string_view url,
                                    const LocalStats& stats) {
  if (ec.category() == curl::errc::GetEasyCategory()) {
    return PrepareEasyException(ec, url, stats);
  } else if (ec.category() == curl::errc::GetRateLimitCategory()) {
    return PrepareRateLimitException(ec, url, stats);
  }

  return std::make_exception_ptr(
      BaseCodeException(ec, "Unknown exception", url, stats));
}

}  // namespace clients::http

USERVER_NAMESPACE_END
