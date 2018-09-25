#include <clients/http/response.hpp>
#include <clients/http/response_future.hpp>

namespace clients {
namespace http {

long Response::status_code() const { return easy_->Easy().get_response_code(); }

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
  curl::easy& easy = const_cast<curl::easy&>(ceasy);

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
