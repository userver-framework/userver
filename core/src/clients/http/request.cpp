#include <clients/http/request.hpp>

#include <chrono>
#include <cstdlib>
#include <map>
#include <string>
#include <system_error>

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/map.hpp>

#include <clients/http/destination_statistics.hpp>
#include <clients/http/error.hpp>
#include <clients/http/form.hpp>
#include <clients/http/request_state.hpp>
#include <clients/http/response_future.hpp>
#include <clients/http/statistics.hpp>
#include <http/common_headers.hpp>
#include <http/url.hpp>
#include <tracing/span.hpp>
#include <tracing/tags.hpp>

#include <clients/http/easy_wrapper.hpp>
#include <clients/http/testsuite.hpp>
#include <crypto/helpers.hpp>
#include <engine/blocking_future.hpp>
#include <engine/ev/watcher/timer_watcher.hpp>

namespace clients::http {

namespace {
/// Max number of retries during calculating timeout
constexpr int kMaxRetryInTimeout = 5;
/// Base time for exponential backoff algorithm
constexpr long kEBBaseTime = 25;

constexpr std::string_view kHeaderExpect = "Expect";

std::string ToString(HttpMethod method) {
  switch (method) {
    case HttpMethod::kDelete:
      return "DELETE";
    case HttpMethod::kGet:
      return "GET";
    case HttpMethod::kHead:
      return "HEAD";
    case HttpMethod::kPost:
      return "POST";
    case HttpMethod::kPut:
      return "PUT";
    case HttpMethod::kPatch:
      return "PATCH";
    case HttpMethod::kOptions:
      return "OPTIONS";
  }
}

curl::easy::http_version_t ToNative(HttpVersion version) {
  switch (version) {
    case HttpVersion::kDefault:
      return curl::easy::http_version_t::http_version_none;
    case HttpVersion::k10:
      return curl::easy::http_version_t::http_version_1_0;
    case HttpVersion::k11:
      return curl::easy::http_version_t::http_version_1_1;
    case HttpVersion::k2:
      return curl::easy::http_version_t::http_version_2_0;
    case HttpVersion::k2Tls:
      return curl::easy::http_version_t::http_vertion_2tls;
    case HttpVersion::k2PriorKnowledge:
      return curl::easy::http_version_t::http_version_2_prior_knowledge;
  }
}

inline long max_retry_time(short number) {
  long time_ms = 0;
  for (short int i = 1; i < number; ++i) {
    time_ms += kEBBaseTime * ((1 << std::min(i - 1, kMaxRetryInTimeout)) + 1);
  }
  return time_ms;
}

long complete_timeout(long request_timeout, short retries) {
  return static_cast<long>(static_cast<double>(request_timeout * retries) *
                           1.1) +
         max_retry_time(retries);
}

bool IsUserAgentHeader(std::string_view header_name) {
  return utils::StrIcaseEqual{}(header_name, ::http::headers::kUserAgent);
}

void SetUserAgent(curl::easy& easy, const std::string& value) {
  easy.set_user_agent(value);
}

void SetUserAgent(curl::easy& easy, std::string_view value) {
  easy.set_user_agent(std::string{value});
}

template <class Range>
void SetHeaders(curl::easy& easy, const Range& headers_range) {
  for (const auto& [name, value] : headers_range) {
    if (!IsUserAgentHeader(name)) {
      easy.add_header(name, value);
    } else {
      clients::http::SetUserAgent(easy, value);
    }
  }
}

}  // namespace

// Request implementation

Request::Request(std::shared_ptr<impl::EasyWrapper>&& wrapper,
                 std::shared_ptr<RequestStats>&& req_stats,
                 const std::shared_ptr<DestinationStatistics>& dest_stats)
    : pimpl_(std::make_shared<RequestState>(std::move(wrapper),
                                            std::move(req_stats), dest_stats)) {
  LOG_DEBUG() << "Request::Request()";
  // default behavior follow redirects and verify ssl
  pimpl_->follow_redirects(true);
  pimpl_->verify(true);

  if (engine::current_task::ShouldCancel()) {
    throw CancelException(
        "Failed to make HTTP request due to task cancellation", {});
  }
}

ResponseFuture Request::async_perform() {
  return ResponseFuture(pimpl_->async_perform(),
                        std::chrono::milliseconds(complete_timeout(
                            pimpl_->timeout(), pimpl_->retries())),
                        pimpl_);
}

std::shared_ptr<Response> Request::perform() { return async_perform().Get(); }

std::shared_ptr<Request> Request::url(const std::string& url) {
  std::error_code ec;
  pimpl_->easy().set_url(url, ec);
  if (ec) throw BadArgumentException(ec, "Bad URL", url, {});

  pimpl_->SetDestinationMetricNameAuto(::http::ExtractMetaTypeFromUrl(url));
  return shared_from_this();
}

std::shared_ptr<Request> Request::timeout(long timeout_ms) {
  pimpl_->set_timeout(timeout_ms);
  return shared_from_this();
}

std::shared_ptr<Request> Request::follow_redirects(bool follow) {
  pimpl_->follow_redirects(follow);
  return shared_from_this();
}

std::shared_ptr<Request> Request::verify(bool verify) {
  pimpl_->verify(verify);
  return shared_from_this();
}

std::shared_ptr<Request> Request::ca_info(const std::string& file_path) {
  pimpl_->ca_info(file_path);
  return shared_from_this();
}

std::shared_ptr<Request> Request::ca_file(const std::string& dir_path) {
  pimpl_->ca_file(dir_path);
  return shared_from_this();
}

std::shared_ptr<Request> Request::crl_file(const std::string& file_path) {
  pimpl_->crl_file(file_path);
  return shared_from_this();
}

std::shared_ptr<Request> Request::client_key_cert(crypto::PrivateKey pkey,
                                                  crypto::Certificate cert) {
  pimpl_->client_key_cert(std::move(pkey), std::move(cert));
  return shared_from_this();
}

std::shared_ptr<Request> Request::http_version(HttpVersion version) {
  pimpl_->http_version(ToNative(version));
  return shared_from_this();
}

std::shared_ptr<Request> Request::retry(int retries, bool on_fails) {
  UASSERT_MSG(retries >= 0, "retires < 0 (" + std::to_string(retries) +
                                "), uninitialized variable?");
  if (retries <= 0) retries = 1;
  pimpl_->retry(retries, on_fails);
  return shared_from_this();
}

std::shared_ptr<Request> Request::data(std::string data) {
  if (!data.empty())
    pimpl_->easy().add_header(kHeaderExpect, "",
                              curl::easy::EmptyHeaderAction::kDoNotSend);
  pimpl_->easy().set_post_fields(std::move(data));
  return shared_from_this();
}

std::shared_ptr<Request> Request::form(const Form& form) {
  pimpl_->easy().set_http_post(form.GetNative());
  pimpl_->easy().add_header(kHeaderExpect, "",
                            curl::easy::EmptyHeaderAction::kDoNotSend);
  return shared_from_this();
}

std::shared_ptr<Request> Request::headers(const Headers& headers) {
  SetHeaders(pimpl_->easy(), headers);
  return shared_from_this();
}

std::shared_ptr<Request> Request::headers(
    std::initializer_list<std::pair<std::string_view, std::string_view>>
        headers) {
  SetHeaders(pimpl_->easy(), headers);
  return shared_from_this();
}

std::shared_ptr<Request> Request::user_agent(const std::string& value) {
  pimpl_->easy().set_user_agent(value.c_str());
  return shared_from_this();
}

std::shared_ptr<Request> Request::cookies(const Cookies& cookies) {
  std::string cookie_str;
  for (const auto& [name, value] : cookies) {
    if (!cookie_str.empty()) cookie_str += "; ";
    cookie_str += name;
    cookie_str += '=';
    cookie_str += value;
  }
  pimpl_->easy().set_cookie(cookie_str);
  return shared_from_this();
}

std::shared_ptr<Request> Request::method(HttpMethod method) {
  switch (method) {
    case HttpMethod::kDelete:
    case HttpMethod::kOptions:
      pimpl_->easy().set_custom_request(ToString(method));
      break;
    case HttpMethod::kGet:
      pimpl_->easy().set_http_get(true);
      break;
    case HttpMethod::kHead:
      pimpl_->easy().set_no_body(true);
      break;
    // NOTE: set_post makes libcURL to read from stdin if no data is set
    case HttpMethod::kPost:
    case HttpMethod::kPut:
    case HttpMethod::kPatch:
      pimpl_->easy().set_custom_request(ToString(method));
      // ensure a body as we should send Content-Length for this method
      if (!pimpl_->easy().has_post_data()) data({});
      break;
  };
  return shared_from_this();
}

std::shared_ptr<Request> Request::get() { return method(HttpMethod::kGet); }

std::shared_ptr<Request> Request::head() { return method(HttpMethod::kHead); }

std::shared_ptr<Request> Request::post() { return method(HttpMethod::kPost); }

std::shared_ptr<Request> Request::put() { return method(HttpMethod::kPut); }

std::shared_ptr<Request> Request::patch() { return method(HttpMethod::kPatch); }

std::shared_ptr<Request> Request::delete_method() {
  return method(HttpMethod::kDelete);
}

std::shared_ptr<Request> Request::get(const std::string& url) {
  return get()->url(url);
}

std::shared_ptr<Request> Request::head(const std::string& url) {
  return head()->url(url);
}

std::shared_ptr<Request> Request::post(const std::string& url,
                                       const Form& form) {
  return this->url(url)->form(form);
}

std::shared_ptr<Request> Request::post(const std::string& url,
                                       std::string data) {
  return this->url(url)->data(std::move(data))->post();
}

std::shared_ptr<Request> Request::put(const std::string& url,
                                      std::string data) {
  return this->url(url)->data(std::move(data))->put();
}

std::shared_ptr<Request> Request::patch(const std::string& url,
                                        std::string data) {
  return this->url(url)->data(std::move(data))->patch();
}

std::shared_ptr<Request> Request::delete_method(const std::string& url) {
  return delete_method()->url(url);
}

std::shared_ptr<Response> Request::response() const {
  return pimpl_->response();
}

std::shared_ptr<Request> Request::SetDestinationMetricName(
    const std::string& destination) {
  pimpl_->SetDestinationMetricName(destination);
  return shared_from_this();
}

std::shared_ptr<Request> Request::SetTestsuiteConfig(
    const std::shared_ptr<const TestsuiteConfig>& config) {
  pimpl_->SetTestsuiteConfig(config);
  return shared_from_this();
}

std::shared_ptr<Request> Request::DisableReplyDecoding() {
  pimpl_->DisableReplyDecoding();
  return shared_from_this();
}

}  // namespace clients::http
