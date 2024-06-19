#include <userver/clients/http/request.hpp>

#include <chrono>
#include <cstdlib>
#include <map>
#include <string>
#include <string_view>
#include <system_error>

#include <userver/clients/http/connect_to.hpp>
#include <userver/clients/http/error.hpp>
#include <userver/clients/http/form.hpp>
#include <userver/clients/http/response_future.hpp>
#include <userver/clients/http/streamed_response.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/future.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/url.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/str_icase.hpp>
#include <userver/utils/trivial_map.hpp>

#include <clients/http/destination_statistics.hpp>
#include <clients/http/easy_wrapper.hpp>
#include <clients/http/request_state.hpp>
#include <clients/http/statistics.hpp>
#include <clients/http/testsuite.hpp>
#include <crypto/helpers.hpp>
#include <engine/ev/watcher/timer_watcher.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

namespace {

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
      return curl::easy::http_version_t::http_version_2tls;
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

curl::easy::httpauth_t HttpAuthTypeToNative(HttpAuthType value) {
  switch (value) {
    case HttpAuthType::kBasic:
      return curl::easy::auth_basic;
    case HttpAuthType::kDigest:
      return curl::easy::auth_digest;
    case HttpAuthType::kDigestIE:
      return curl::easy::auth_digest_ie;
    case HttpAuthType::kNegotiate:
      return curl::easy::auth_negotiate;
    case HttpAuthType::kNtlm:
      return curl::easy::auth_ntlm;
    case HttpAuthType::kNtlmWb:
      return curl::easy::auth_ntlm_wb;
    case HttpAuthType::kAny:
      return curl::easy::auth_any;
    case HttpAuthType::kAnySafe:
      return curl::easy::auth_any_safe;
  }

  UINVARIANT(false, "Unexpected http auth type");
}

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

  for (const std::string_view allowed_schema : kAllowedSchemas) {
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

Request::Request(impl::EasyWrapper&& wrapper, RequestStats&& req_stats,
                 const std::shared_ptr<DestinationStatistics>& dest_stats,
                 clients::dns::Resolver* resolver,
                 impl::PluginPipeline& plugin_pipeline,
                 const tracing::TracingManagerBase& tracing_manager)
    : pimpl_(std::make_shared<RequestState>(
          std::move(wrapper), std::move(req_stats), dest_stats, resolver,
          plugin_pipeline, tracing_manager)) {
  LOG_TRACE() << "Request::Request()";
  // default behavior follow redirects and verify ssl
  pimpl_->follow_redirects(true);
  pimpl_->verify(true);

  if (engine::current_task::ShouldCancel()) {
    throw CancelException(
        "Failed to make HTTP request due to task cancellation", {},
        ErrorKind::kCancel);
  }
}

ResponseFuture Request::async_perform(utils::impl::SourceLocation location) {
  ResponseFuture future{pimpl_->async_perform(location), pimpl_};
  return future;
}

StreamedResponse Request::async_perform_stream_body(
    const std::shared_ptr<concurrent::StringStreamQueue>& queue,
    utils::impl::SourceLocation location) {
  LOG_DEBUG() << "Starting an async HTTP request with streamed response body";
  return StreamedResponse(pimpl_->async_perform_stream(queue, location),
                          queue->GetConsumer(), pimpl_);
}

std::shared_ptr<Response> Request::perform(
    utils::impl::SourceLocation location) {
  return async_perform(location).Get();
}

Request& Request::url(const std::string& url) & {
  if (!IsAllowedSchemaInUrl(url)) {
    throw BadArgumentException(curl::errc::EasyErrorCode::kUnsupportedProtocol,
                               "Bad URL", url, {});
  }
  std::error_code ec;
  pimpl_->easy().set_url(url, ec);
  if (ec) throw BadArgumentException(ec, "Bad URL", url, {});

  pimpl_->SetDestinationMetricNameAuto(
      USERVER_NAMESPACE::http::ExtractMetaTypeFromUrl(url));
  return *this;
}
Request Request::url(const std::string& url) && {
  return std::move(this->url(url));
}

Request& Request::timeout(long timeout_ms) & {
  pimpl_->set_timeout(timeout_ms);
  return *this;
}
Request Request::timeout(long timeout_ms) && {
  return std::move(this->timeout(timeout_ms));
}

Request& Request::follow_redirects(bool follow) & {
  pimpl_->follow_redirects(follow);
  return *this;
}
Request Request::follow_redirects(bool follow) && {
  return std::move(this->follow_redirects(follow));
}

Request& Request::verify(bool verify) & {
  pimpl_->verify(verify);
  return *this;
}
Request Request::verify(bool verify) && {
  return std::move(this->verify(verify));
}

Request& Request::ca_info(const std::string& file_path) & {
  pimpl_->ca_info(file_path);
  return *this;
}
Request Request::ca_info(const std::string& file_path) && {
  return std::move(this->ca_info(file_path));
}

Request& Request::ca(crypto::Certificate cert) & {
  pimpl_->ca(std::move(cert));
  return *this;
}
Request Request::ca(crypto::Certificate cert) && {
  return std::move(this->ca(std::move(cert)));
}

Request& Request::crl_file(const std::string& file_path) & {
  pimpl_->crl_file(file_path);
  return *this;
}
Request Request::crl_file(const std::string& file_path) && {
  return std::move(this->crl_file(file_path));
}

Request& Request::client_key_cert(crypto::PrivateKey pkey,
                                  crypto::Certificate cert) & {
  pimpl_->client_key_cert(std::move(pkey), std::move(cert));
  return *this;
}
Request Request::client_key_cert(crypto::PrivateKey pkey,
                                 crypto::Certificate cert) && {
  return std::move(this->client_key_cert(std::move(pkey), std::move(cert)));
}

Request& Request::http_version(HttpVersion version) & {
  pimpl_->http_version(ToNative(version));
  return *this;
}
Request Request::http_version(HttpVersion version) && {
  return std::move(this->http_version(version));
}

Request& Request::retry(short retries, bool on_fails) & {
  UASSERT_MSG(retries >= 0, "retires < 0 (" + std::to_string(retries) +
                                "), uninitialized variable?");
  if (retries <= 0) retries = 1;
  pimpl_->retry(retries, on_fails);
  return *this;
}
Request Request::retry(short retries, bool on_fails) && {
  return std::move(this->retry(retries, on_fails));
}

Request& Request::unix_socket_path(const std::string& path) & {
  pimpl_->unix_socket_path(path);
  return *this;
}
Request Request::unix_socket_path(const std::string& path) && {
  return std::move(this->unix_socket_path(path));
}

Request& Request::use_ipv4() & {
  pimpl_->easy().set_ip_resolve(curl::easy::ip_resolve_v4);
  return *this;
}
Request Request::use_ipv4() && { return std::move(this->use_ipv4()); }

Request& Request::use_ipv6() & {
  pimpl_->easy().set_ip_resolve(curl::easy::ip_resolve_v6);
  return *this;
}
Request Request::use_ipv6() && { return std::move(this->use_ipv6()); }

Request& Request::connect_to(const ConnectTo& connect_to) & {
  pimpl_->connect_to(connect_to);
  return *this;
}
Request Request::connect_to(const ConnectTo& connect_to) && {
  return std::move(this->connect_to(connect_to));
}

Request& Request::data(std::string data) & {
  if (!data.empty())
    pimpl_->easy().add_header(kHeaderExpect, "",
                              curl::easy::EmptyHeaderAction::kDoNotSend);
  pimpl_->easy().set_post_fields(std::move(data));
  return *this;
}
Request Request::data(std::string data) && {
  return std::move(this->data(std::move(data)));
}

Request& Request::form(Form&& form) & {
  pimpl_->easy().set_http_post(std::move(form).GetNative());
  pimpl_->easy().add_header(kHeaderExpect, "",
                            curl::easy::EmptyHeaderAction::kDoNotSend);
  return *this;
}
Request Request::form(Form&& form) && {
  return std::move(this->form(std::move(form)));
}

Request& Request::headers(const Headers& headers) & {
  SetHeaders(pimpl_->easy(), headers);
  return *this;
}
Request Request::headers(const Headers& headers) && {
  return std::move(this->headers(headers));
}

Request& Request::headers(
    const std::initializer_list<std::pair<std::string_view, std::string_view>>&
        headers) & {
  SetHeaders(pimpl_->easy(), headers);
  return *this;
}
Request Request::headers(
    const std::initializer_list<std::pair<std::string_view, std::string_view>>&
        headers) && {
  return std::move(this->headers(headers));
}

Request& Request::http_auth_type(HttpAuthType value, bool auth_only,
                                 std::string_view user,
                                 std::string_view password) & {
  pimpl_->http_auth_type(HttpAuthTypeToNative(value), auth_only, user,
                         password);
  return *this;
}
Request Request::http_auth_type(HttpAuthType value, bool auth_only,
                                std::string_view user,
                                std::string_view password) && {
  return std::move(this->http_auth_type(value, auth_only, user, password));
}

Request& Request::proxy_headers(const Headers& headers) & {
  SetProxyHeaders(pimpl_->easy(), headers);
  return *this;
}
Request Request::proxy_headers(const Headers& headers) && {
  return std::move(this->proxy_headers(headers));
}

Request& Request::proxy_headers(
    const std::initializer_list<std::pair<std::string_view, std::string_view>>&
        headers) & {
  SetProxyHeaders(pimpl_->easy(), headers);
  return *this;
}
Request Request::proxy_headers(
    const std::initializer_list<std::pair<std::string_view, std::string_view>>&
        headers) && {
  return std::move(this->proxy_headers(headers));
}

Request& Request::user_agent(const std::string& value) & {
  pimpl_->easy().set_user_agent(value.c_str());
  return *this;
}
Request Request::user_agent(const std::string& value) && {
  return std::move(this->user_agent(value));
}

Request& Request::proxy(const std::string& value) & {
  pimpl_->proxy(value);
  return *this;
}
Request Request::proxy(const std::string& value) && {
  return std::move(this->proxy(value));
}

Request& Request::proxy_auth_type(ProxyAuthType value) & {
  pimpl_->proxy_auth_type(ProxyAuthTypeToNative(value));
  return *this;
}
Request Request::proxy_auth_type(ProxyAuthType value) && {
  return std::move(this->proxy_auth_type(value));
}

Request& Request::cookies(const Cookies& cookies) & {
  SetCookies(pimpl_->easy(), cookies);
  return *this;
}
Request Request::cookies(const Cookies& cookies) && {
  return std::move(this->cookies(cookies));
}

Request& Request::cookies(
    const std::unordered_map<std::string, std::string>& cookies) & {
  SetCookies(pimpl_->easy(), cookies);
  return *this;
}
Request Request::cookies(
    const std::unordered_map<std::string, std::string>& cookies) && {
  return std::move(this->cookies(cookies));
}

Request& Request::method(HttpMethod method) & {
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
  return *this;
}

Request Request::method(HttpMethod method) && {
  return std::move(this->method(method));
}

Request& Request::get() & { return method(HttpMethod::kGet); }
Request Request::get() && { return std::move(this->get()); }

Request& Request::head() & { return method(HttpMethod::kHead); }
Request Request::head() && { return std::move(this->head()); }

Request& Request::post() & { return method(HttpMethod::kPost); }
Request Request::post() && { return std::move(this->post()); }

Request& Request::put() & { return method(HttpMethod::kPut); }
Request Request::put() && { return std::move(this->put()); }

Request& Request::patch() & { return method(HttpMethod::kPatch); }
Request Request::patch() && { return std::move(this->patch()); }

Request& Request::delete_method() & { return method(HttpMethod::kDelete); }
Request Request::delete_method() && { return std::move(this->delete_method()); }

Request& Request::set_custom_http_request_method(std::string method) & {
  LOG_LIMITED_WARNING()
      << "This method can cause unexpected effects in libcurl, i.e., timeouts, "
         "changing of request type. Use it only if you need to make "
         "GET-request with body.";
  pimpl_->easy().set_custom_request(method);
  return *this;
}
Request Request::set_custom_http_request_method(std::string method) && {
  return std::move(this->set_custom_http_request_method(std::move(method)));
}

Request& Request::get(const std::string& url) & { return get().url(url); }
Request Request::get(const std::string& url) && {
  return std::move(this->get(url));
}

Request& Request::head(const std::string& url) & { return head().url(url); }
Request Request::head(const std::string& url) && {
  return std::move(this->head(url));
}

Request& Request::post(const std::string& url, Form&& form) & {
  return this->url(url).form(std::move(form));
}
Request Request::post(const std::string& url, Form&& form) && {
  return std::move(this->post(url, std::move(form)));
}

Request& Request::post(const std::string& url, std::string data) & {
  return this->url(url).data(std::move(data)).post();
}
Request Request::post(const std::string& url, std::string data) && {
  return std::move(this->post(url, std::move(data)));
}

Request& Request::put(const std::string& url, std::string data) & {
  return this->url(url).data(std::move(data)).put();
}
Request Request::put(const std::string& url, std::string data) && {
  return std::move(this->put(url, std::move(data)));
}

Request& Request::patch(const std::string& url, std::string data) & {
  return this->url(url).data(std::move(data)).patch();
}
Request Request::patch(const std::string& url, std::string data) && {
  return std::move(this->patch(url, std::move(data)));
}

Request& Request::delete_method(const std::string& url) & {
  return this->url(url).delete_method();
}
Request Request::delete_method(const std::string& url) && {
  return std::move(this->delete_method(url));
}

Request& Request::delete_method(const std::string& url, std::string data) & {
  return this->url(url).data(std::move(data)).delete_method();
}
Request Request::delete_method(const std::string& url, std::string data) && {
  return std::move(this->delete_method(url, std::move(data)));
}

Request& Request::SetLoggedUrl(std::string url) & {
  pimpl_->SetLoggedUrl(std::move(url));
  return *this;
}
Request Request::SetLoggedUrl(std::string url) && {
  return std::move(this->SetLoggedUrl(std::move(url)));
}

Request& Request::SetDestinationMetricName(const std::string& destination) & {
  pimpl_->SetDestinationMetricName(destination);
  return *this;
}
Request Request::SetDestinationMetricName(const std::string& destination) && {
  return std::move(this->SetDestinationMetricName(destination));
}

void Request::SetTestsuiteConfig(
    const std::shared_ptr<const TestsuiteConfig>& config) & {
  pimpl_->SetTestsuiteConfig(config);
}

void Request::SetAllowedUrlsExtra(const std::vector<std::string>& urls) & {
  pimpl_->SetAllowedUrlsExtra(urls);
}

void Request::SetDeadlinePropagationConfig(
    const DeadlinePropagationConfig& deadline_propagation_config) & {
  pimpl_->SetDeadlinePropagationConfig(deadline_propagation_config);
}

Request& Request::DisableReplyDecoding() & {
  pimpl_->DisableReplyDecoding();
  return *this;
}
Request Request::DisableReplyDecoding() && {
  return std::move(this->DisableReplyDecoding());
}

void Request::SetCancellationPolicy(CancellationPolicy cp) {
  pimpl_->SetCancellationPolicy(cp);
}

Request& Request::SetTracingManager(
    const tracing::TracingManagerBase& tracing_manager) & {
  pimpl_->SetTracingManager(tracing_manager);
  return *this;
}
Request Request::SetTracingManager(
    const tracing::TracingManagerBase& tracing_manager) && {
  return std::move(this->SetTracingManager(tracing_manager));
}

void Request::SetHeadersPropagator(
    const server::http::HeadersPropagator* headers_propagator) & {
  pimpl_->SetHeadersPropagator(headers_propagator);
}

const std::string& Request::GetUrl() const& {
  return pimpl_->easy().get_original_url();
}

const std::string& Request::GetData() const& {
  return pimpl_->easy().get_post_data();
}

std::string Request::ExtractData() {
  return pimpl_->easy().extract_post_data();
}

}  // namespace clients::http

USERVER_NAMESPACE_END
