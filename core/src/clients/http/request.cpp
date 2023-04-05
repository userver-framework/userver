#include <userver/clients/http/request.hpp>

#include <chrono>
#include <cstdlib>
#include <map>
#include <string>
#include <string_view>
#include <system_error>

#include <userver/clients/http/error.hpp>
#include <userver/clients/http/form.hpp>
#include <userver/clients/http/response_future.hpp>
#include <userver/clients/http/streamed_response.hpp>
#include <userver/engine/future.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/url.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/str_icase.hpp>
#include <userver/utils/trivial_map.hpp>

#include <clients/http/destination_statistics.hpp>
#include <clients/http/easy_wrapper.hpp>
#include <clients/http/request_state.hpp>
#include <clients/http/statistics.hpp>
#include <clients/http/testsuite.hpp>
#include <crypto/helpers.hpp>
#include <engine/ev/watcher/timer_watcher.hpp>
#include <utils/impl/assert_extra.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

namespace {
/// Max number of retries during calculating timeout
constexpr int kMaxRetryInTimeout = 5;
/// Base time for exponential backoff algorithm
constexpr long kEBBaseTime = 25;

constexpr std::string_view kHeaderExpect = "Expect";

std::string ToString(HttpMethod method) {
  return std::string{ToStringView(method)};
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

  UINVARIANT(false, "Unexpected HTTP version");
}

constexpr utils::TrivialBiMap kAuthTypeMap = [](auto selector) {
  return selector()
      .Case("basic", ProxyAuthType::kBasic)
      .Case("digest", ProxyAuthType::kDigest)
      .Case("digest_ie", ProxyAuthType::kDigestIE)
      .Case("bearer", ProxyAuthType::kBearer)
      .Case("negotiate", ProxyAuthType::kNegotiate)
      .Case("ntlm", ProxyAuthType::kNtlm)
      .Case("ntlm_wb", ProxyAuthType::kNtlmWb)
      .Case("any", ProxyAuthType::kAny)
      .Case("any_safe", ProxyAuthType::kAnySafe);
};

curl::easy::proxyauth_t ProxyAuthTypeToNative(ProxyAuthType value) {
  switch (value) {
    case ProxyAuthType::kBasic:
      return curl::easy::proxy_auth_basic;
    case ProxyAuthType::kDigest:
      return curl::easy::proxy_auth_digest;
    case ProxyAuthType::kDigestIE:
      return curl::easy::proxy_auth_digest_ie;
    case ProxyAuthType::kBearer:
      return curl::easy::proxy_auth_bearer;
    case ProxyAuthType::kNegotiate:
      return curl::easy::proxy_auth_negotiate;
    case ProxyAuthType::kNtlm:
      return curl::easy::proxy_auth_ntlm;
    case ProxyAuthType::kNtlmWb:
      return curl::easy::proxy_auth_ntlm_wb;
    case ProxyAuthType::kAny:
      return curl::easy::proxy_auth_any;
    case ProxyAuthType::kAnySafe:
      return curl::easy::proxy_auth_anysafe;
  }

  UINVARIANT(false, "Unexpected proxy auth type");
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
  return utils::StrIcaseEqual{}(header_name,
                                USERVER_NAMESPACE::http::headers::kUserAgent);
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

template <class Range>
void SetCookies(curl::easy& easy, const Range& cookies_range) {
  std::string cookie_str;
  for (const auto& [name, value] : cookies_range) {
    if (!cookie_str.empty()) cookie_str += "; ";
    cookie_str += name;
    cookie_str += '=';
    cookie_str += value;
  }
  easy.set_cookie(cookie_str);
}

template <class Range>
void SetProxyHeaders(curl::easy& easy, const Range& headers_range) {
  for (const auto& [name, value] : headers_range) {
    easy.add_proxy_header(name, value);
  }
}

bool IsAllowedSchemaInUrl(std::string_view url) {
  static constexpr std::string_view kAllowedSchemas[] = {"http://", "https://"};

  for (std::string_view allowed_schema : kAllowedSchemas) {
    if (utils::StrIcaseEqual{}(allowed_schema,
                               url.substr(0, allowed_schema.size()))) {
      return true;
    }
  }
  return false;
}

}  // namespace

std::string_view ToStringView(HttpMethod method) {
  static constexpr utils::TrivialBiMap kMap([](auto selector) {
    return selector()
        .Case(HttpMethod::kDelete, "DELETE")
        .Case(HttpMethod::kGet, "GET")
        .Case(HttpMethod::kHead, "HEAD")
        .Case(HttpMethod::kPost, "POST")
        .Case(HttpMethod::kPut, "PUT")
        .Case(HttpMethod::kPatch, "PATCH")
        .Case(HttpMethod::kOptions, "OPTIONS");
  });

  return utils::impl::EnumToStringView(method, kMap);
}

ProxyAuthType ProxyAuthTypeFromString(const std::string& auth_name) {
  auto value = kAuthTypeMap.TryFindICase(auth_name);
  if (!value) {
    throw std::runtime_error(
        fmt::format("Unknown proxy auth type '{}' (must be one of {})",
                    auth_name, kAuthTypeMap.DescribeFirst()));
  }
  return *value;
}

// Request implementation

Request::Request(std::shared_ptr<impl::EasyWrapper>&& wrapper,
                 std::shared_ptr<RequestStats>&& req_stats,
                 const std::shared_ptr<DestinationStatistics>& dest_stats,
                 clients::dns::Resolver* resolver,
                 impl::PluginPipeline& plugin_pipeline)
    : pimpl_(std::make_shared<RequestState>(std::move(wrapper),
                                            std::move(req_stats), dest_stats,
                                            resolver, plugin_pipeline)) {
  LOG_TRACE() << "Request::Request()";
  // default behavior follow redirects and verify ssl
  pimpl_->follow_redirects(true);
  pimpl_->verify(true);

  if (engine::current_task::ShouldCancel()) {
    throw CancelException(
        "Failed to make HTTP request due to task cancellation", {});
  }
}

ResponseFuture Request::async_perform() {
  return {pimpl_->async_perform(),
          std::chrono::milliseconds(
              complete_timeout(pimpl_->timeout(), pimpl_->retries())),
          pimpl_};
}

StreamedResponse Request::async_perform_stream_body(
    const std::shared_ptr<concurrent::SpscQueue<std::string>>& queue) {
  LOG_DEBUG() << "Starting an async HTTP request with streamed response body";
  pimpl_->async_perform_stream(queue);
  auto deadline = engine::Deadline::FromDuration(
      std::chrono::milliseconds(pimpl_->effective_timeout()));
  return StreamedResponse(queue->GetConsumer(), deadline, pimpl_);
}

std::shared_ptr<Response> Request::perform() { return async_perform().Get(); }

std::shared_ptr<Request> Request::url(const std::string& url) {
  if (!IsAllowedSchemaInUrl(url)) {
    throw BadArgumentException(curl::errc::EasyErrorCode::kUnsupportedProtocol,
                               "Bad URL", url, {});
  }
  std::error_code ec;
  pimpl_->easy().set_url(url, ec);
  if (ec) throw BadArgumentException(ec, "Bad URL", url, {});

  pimpl_->SetDestinationMetricNameAuto(
      USERVER_NAMESPACE::http::ExtractMetaTypeFromUrl(url));
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

std::shared_ptr<Request> Request::ca(crypto::Certificate cert) {
  pimpl_->ca(std::move(cert));
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

std::shared_ptr<Request> Request::retry(short retries, bool on_fails) {
  UASSERT_MSG(retries >= 0, "retires < 0 (" + std::to_string(retries) +
                                "), uninitialized variable?");
  if (retries <= 0) retries = 1;
  pimpl_->retry(retries, on_fails);
  return shared_from_this();
}

std::shared_ptr<Request> Request::unix_socket_path(const std::string& path) {
  pimpl_->unix_socket_path(path);
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

std::shared_ptr<Request> Request::proxy_headers(const Headers& headers) {
  SetProxyHeaders(pimpl_->easy(), headers);
  return shared_from_this();
}

std::shared_ptr<Request> Request::proxy_headers(
    std::initializer_list<std::pair<std::string_view, std::string_view>>
        headers) {
  SetProxyHeaders(pimpl_->easy(), headers);
  return shared_from_this();
}

std::shared_ptr<Request> Request::user_agent(const std::string& value) {
  pimpl_->easy().set_user_agent(value.c_str());
  return shared_from_this();
}

std::shared_ptr<Request> Request::proxy(const std::string& value) {
  pimpl_->proxy(value);
  return shared_from_this();
}

std::shared_ptr<Request> Request::proxy_auth_type(ProxyAuthType value) {
  pimpl_->proxy_auth_type(ProxyAuthTypeToNative(value));
  return shared_from_this();
}

std::shared_ptr<Request> Request::cookies(const Cookies& cookies) {
  SetCookies(pimpl_->easy(), cookies);
  return shared_from_this();
}

std::shared_ptr<Request> Request::cookies(
    const std::unordered_map<std::string, std::string>& cookies) {
  SetCookies(pimpl_->easy(), cookies);
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
      pimpl_->easy().set_custom_request(nullptr);
      break;
    case HttpMethod::kHead:
      pimpl_->easy().set_no_body(true);
      pimpl_->easy().set_custom_request(nullptr);
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

std::shared_ptr<Request> Request::set_custom_http_request_method(
    std::string method) {
  LOG_LIMITED_WARNING()
      << "This method can cause unexpected effects in libcurl, i.e., timeouts, "
         "changing of request type. Use it only if you need to make "
         "GET-request with body.";
  pimpl_->easy().set_custom_request(method);
  return shared_from_this();
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
  return this->url(url)->delete_method();
}

std::shared_ptr<Request> Request::delete_method(const std::string& url,
                                                std::string data) {
  return this->url(url)->data(std::move(data))->delete_method();
}

std::shared_ptr<Request> Request::SetLoggedUrl(std::string url) {
  pimpl_->SetLoggedUrl(std::move(url));
  return shared_from_this();
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

std::shared_ptr<Request> Request::SetAllowedUrlsExtra(
    const std::vector<std::string>& urls) {
  pimpl_->SetAllowedUrlsExtra(urls);
  return shared_from_this();
}

std::shared_ptr<Request> Request::DisableReplyDecoding() {
  pimpl_->DisableReplyDecoding();
  return shared_from_this();
}

std::shared_ptr<Request> Request::EnableAddClientTimeoutHeader() {
  pimpl_->EnableAddClientTimeoutHeader();
  return shared_from_this();
}

std::shared_ptr<Request> Request::DisableAddClientTimeoutHeader() {
  pimpl_->DisableAddClientTimeoutHeader();
  return shared_from_this();
}

std::shared_ptr<Request> Request::SetTracingManager(
    const tracing::TracingManagerBase& tracing_manager) {
  pimpl_->SetTracingManager(tracing_manager);
  return shared_from_this();
}

std::shared_ptr<Request> Request::SetHeadersPropagator(
    const server::http::HeadersPropagator* headers_propagator) {
  pimpl_->SetHeadersPropagator(headers_propagator);
  return shared_from_this();
}

std::shared_ptr<Request> Request::SetEnforceTaskDeadline(
    EnforceTaskDeadlineConfig enforce_task_deadline) {
  pimpl_->SetEnforceTaskDeadline(enforce_task_deadline);
  return shared_from_this();
}

const std::string& Request::GetUrl() const {
  return pimpl_->easy().get_original_url();
}

const std::string& Request::GetData() const {
  return pimpl_->easy().get_post_data();
}

std::string Request::ExtractData() {
  return pimpl_->easy().extract_post_data();
}

}  // namespace clients::http

USERVER_NAMESPACE_END
