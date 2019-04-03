#include <clients/http/response.hpp>
#include <clients/http/response_future.hpp>

namespace clients {
namespace http {

std::ostream& operator<<(std::ostream& os, Status s) {
  switch (s) {
    case OK:
      return os << "200 OK";
    case Created:
      return os << "201 Created";
    case BadRequest:
      return os << "400 BadRequest";
    case NotFound:
      return os << "404 Not Found";
    case Conflict:
      return os << "409 Conflict";
    case InternalServerError:
      return os << "500 Internal Server Error";
    case BadGateway:
      return os << "502 Bad Gateway";
    case ServiceUnavailable:
      return os << "503 Service Unavailable";
    case GatewayTimeout:
      return os << "504 Gateway Timeout";
    case InsufficientStorage:
      return os << "507 Insufficient Storage";
    case BandwithLimitExceeded:
      return os << "509 Bandwith Limit Exceeded";
    case WebServerIsDown:
      return os << "520 Web Server Is Down";
    case ConnectionTimedOut:
      return os << "522 Connection Timed Out";
    case OriginIsUnreachable:
      return os << "523 Origin Is Unreachable";
    case TimeoutOccured:
      return os << "524 A Timeout Occured";
    case SslHandshakeFailed:
      return os << "525 SSL Handshake Failed";
    case InvalidSslCertificate:
      return os << "526 Invalid SSL Certificate";
  }

  return os << static_cast<std::underlying_type_t<Status>>(s);
}

Status Response::status_code() const {
  return static_cast<Status>(easy_->Easy().get_response_code());
}

double Response::total_time() const { return easy_->Easy().get_total_time(); }

double Response::connect_time() const {
  return easy_->Easy().get_connect_time();
}

double Response::name_lookup_time() const {
  return easy_->Easy().get_namelookup_time();
}

double Response::appconnect_time() const {
  return easy_->Easy().get_appconnect_time();
}

double Response::pretransfer_time() const {
  return easy_->Easy().get_pretransfer_time();
}

double Response::starttransfer_time() const {
  return easy_->Easy().get_starttransfer_time();
}

void Response::RaiseForStatus(long code) {
  if (400 <= code && code < 500)
    throw HttpClientException(code);
  else if (500 <= code && code < 600)
    throw HttpServerException(code);
}

void Response::raise_for_status() const { RaiseForStatus(status_code()); }

curl::easy& Response::easy() { return easy_->Easy(); }

const curl::easy& Response::easy() const { return easy_->Easy(); }

curl::LocalTimings Response::local_timings() const {
  return easy_->Easy().timings();
}

}  // namespace http
}  // namespace clients

std::basic_ostream<char, std::char_traits<char>>& curl::operator<<(
    std::ostream& stream, const curl::easy& ceasy) {
  // curl::easy does not have const methods, but we really do not change it here
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  auto& easy = const_cast<curl::easy&>(ceasy);

  // url code upload download time
  stream << easy.get_effective_url() << ' ' << easy.get_response_code() << ' '
         << static_cast<int64_t>(easy.get_content_length_upload()) << ' '
         << static_cast<int64_t>(easy.get_content_length_download()) << ' '
         << easy.get_total_time();
  return stream;
}

std::ostream& operator<<(std::ostream& stream,
                         const clients::http::Response& response) {
  return operator<<(stream, response.easy());
}
