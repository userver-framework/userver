#pragma once

/// @file userver/clients/http/request.hpp
/// @brief @copybrief clients::http::Request

#include <memory>
#include <string_view>
#include <vector>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/clients/http/error.hpp>
#include <userver/clients/http/plugin.hpp>
#include <userver/clients/http/response.hpp>
#include <userver/clients/http/response_future.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/crypto/certificate.hpp>
#include <userver/crypto/private_key.hpp>
#include <userver/utils/impl/source_location.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {
class TracingManagerBase;
}  // namespace tracing

namespace server::http {
class HeadersPropagator;
}  // namespace server::http

/// HTTP client helpers
namespace clients::http {

class RequestState;
class StreamedResponse;
class ConnectTo;

namespace impl {
class EasyWrapper;
}  // namespace impl

/// HTTP request method
enum class HttpMethod { kGet, kPost, kHead, kPut, kDelete, kPatch, kOptions };

std::string_view ToStringView(HttpMethod method);

/// HTTP version to use
enum class HttpVersion {
  kDefault,  ///< unspecified version
  k10,       ///< HTTP/1.0 only
  k11,       ///< HTTP/1.1 only
  k2,        ///< HTTP/2 with fallback to HTTP/1.1
  k2Tls,     ///< HTTP/2 over TLS only, otherwise (no TLS or h2) HTTP/1.1
  k2PriorKnowledge,  ///< HTTP/2 only (without Upgrade)
};

enum class ProxyAuthType {
  kBasic,      ///< "basic"
  kDigest,     ///< "digest"
  kDigestIE,   ///< "digest_ie"
  kBearer,     ///< "bearer"
  kNegotiate,  ///< "negotiate"
  kNtlm,       ///< "ntlm"
  kNtlmWb,     ///< "ntlm_wb"
  kAny,        ///< "any"
  kAnySafe,    ///< "any_safe"
};

ProxyAuthType ProxyAuthTypeFromString(const std::string& auth_name);

class Form;
class RequestStats;
class DestinationStatistics;
struct TestsuiteConfig;
struct EnforceTaskDeadlineConfig;

/// Class for creating and performing new http requests
class Request final {
 public:
  /// Request cookies container type
  using Cookies =
      std::unordered_map<std::string, std::string, utils::StrCaseHash>;

  /// @cond
  // For internal use only.
  explicit Request(std::shared_ptr<impl::EasyWrapper>&&,
                   std::shared_ptr<RequestStats>&& req_stats,
                   const std::shared_ptr<DestinationStatistics>& dest_stats,
                   clients::dns::Resolver* resolver,
                   impl::PluginPipeline& plugin_pipeline);
  /// @endcond

  /// Specifies method
  Request& method(HttpMethod method) &;
  Request method(HttpMethod method) &&;
  /// GET request
  Request& get() &;
  Request get() &&;
  /// GET request with url
  Request& get(const std::string& url) &;
  Request get(const std::string& url) &&;
  /// HEAD request
  Request& head() &;
  Request head() &&;
  /// HEAD request with url
  Request& head(const std::string& url) &;
  Request head(const std::string& url) &&;
  /// POST request
  Request& post() &;
  Request post() &&;
  /// POST request with url and data
  Request& post(const std::string& url, std::string data = {}) &;
  Request post(const std::string& url, std::string data = {}) &&;
  /// POST request with url and multipart/form-data
  Request& post(const std::string& url, const Form& form) &;
  Request post(const std::string& url, const Form& form) &&;
  /// PUT request
  Request& put() &;
  Request put() &&;
  /// PUT request with url and data
  Request& put(const std::string& url, std::string data = {}) &;
  Request put(const std::string& url, std::string data = {}) &&;

  /// PATCH request
  Request& patch() &;
  Request patch() &&;
  /// PATCH request with url and data
  Request& patch(const std::string& url, std::string data = {}) &;
  Request patch(const std::string& url, std::string data = {}) &&;

  /// DELETE request
  Request& delete_method() &;
  Request delete_method() &&;
  /// DELETE request with url
  Request& delete_method(const std::string& url) &;
  Request delete_method(const std::string& url) &&;
  /// DELETE request with url and data
  Request& delete_method(const std::string& url, std::string data) &;
  Request delete_method(const std::string& url, std::string data) &&;

  /// Set custom request method. Only replaces name of the HTTP method
  Request& set_custom_http_request_method(std::string method) &;
  Request set_custom_http_request_method(std::string method) &&;

  /// url if you don't specify request type with url
  Request& url(const std::string& url) &;
  Request url(const std::string& url) &&;
  /// data for POST request
  Request& data(std::string data) &;
  Request data(std::string data) &&;
  /// form for POST request
  Request& form(const Form& form) &;
  Request form(const Form& form) &&;
  /// Headers for request as map
  Request& headers(const Headers& headers) &;
  Request headers(const Headers& headers) &&;
  /// Headers for request as list
  Request& headers(const std::initializer_list<
                   std::pair<std::string_view, std::string_view>>& headers) &;
  Request headers(const std::initializer_list<
                  std::pair<std::string_view, std::string_view>>& headers) &&;
  /// Proxy headers for request as map
  Request& proxy_headers(const Headers& headers) &;
  Request proxy_headers(const Headers& headers) &&;
  /// Proxy headers for request as list
  Request& proxy_headers(
      const std::initializer_list<
          std::pair<std::string_view, std::string_view>>& headers) &;
  Request proxy_headers(
      const std::initializer_list<
          std::pair<std::string_view, std::string_view>>& headers) &&;
  /// Sets the User-Agent header
  Request& user_agent(const std::string& value) &;
  Request user_agent(const std::string& value) &&;
  /// Sets proxy to use. Example: [::1]:1080
  Request& proxy(const std::string& value) &;
  Request proxy(const std::string& value) &&;
  /// Sets proxy auth type to use.
  Request& proxy_auth_type(ProxyAuthType value) &;
  Request proxy_auth_type(ProxyAuthType value) &&;
  /// Cookies for request as HashDos-safe map
  Request& cookies(const Cookies& cookies) &;
  Request cookies(const Cookies& cookies) &&;
  /// Cookies for request as map
  Request& cookies(
      const std::unordered_map<std::string, std::string>& cookies) &;
  Request cookies(
      const std::unordered_map<std::string, std::string>& cookies) &&;
  /// Follow redirects or not. Default: follow
  Request& follow_redirects(bool follow = true) &;
  Request follow_redirects(bool follow = true) &&;
  /// Set timeout in ms for request
  Request& timeout(long timeout_ms) &;
  Request timeout(long timeout_ms) &&;
  Request& timeout(std::chrono::milliseconds timeout_ms) & {
    return timeout(timeout_ms.count());
  }
  Request timeout(std::chrono::milliseconds timeout_ms) && {
    return std::move(this->timeout(timeout_ms.count()));
  }
  /// Verify host and peer or not. Default: verify
  Request& verify(bool verify = true) &;
  Request verify(bool verify = true) &&;
  /// Set file holding one or more certificates to verify the peer with
  Request& ca_info(const std::string& file_path) &;
  Request ca_info(const std::string& file_path) &&;
  /// Set CA
  Request& ca(crypto::Certificate cert) &;
  Request ca(crypto::Certificate cert) &&;
  /// Set CRL-file
  Request& crl_file(const std::string& file_path) &;
  Request crl_file(const std::string& file_path) &&;
  /// Set private client key and certificate for request.
  ///
  /// @warning Do not use this function on MacOS as it may cause Segmentation
  /// Fault on that platform.
  Request& client_key_cert(crypto::PrivateKey pkey, crypto::Certificate cert) &;
  Request client_key_cert(crypto::PrivateKey pkey, crypto::Certificate cert) &&;
  /// Set HTTP version
  Request& http_version(HttpVersion version) &;
  Request http_version(HttpVersion version) &&;

  /// Specify number of retries on incorrect status, if on_fails is True
  /// retry on network error too. Retries = 3 means that maximum 3 request
  /// will be performed.
  ///
  /// Retries use exponential backoff - an exponentially increasing delay
  /// is added before each retry of this request.
  Request& retry(short retries = 3, bool on_fails = true) &;
  Request retry(short retries = 3, bool on_fails = true) &&;

  /// Set unix domain socket as connection endpoint and provide path to it
  /// When enabled, request will connect to the Unix domain socket instead
  /// of establishing a TCP connection to a host.
  Request& unix_socket_path(const std::string& path) &;
  Request unix_socket_path(const std::string& path) &&;

  /// Set CURLOPT_CONNECT_TO option
  /// @warning connect_to argument must outlive Request
  Request& connect_to(const ConnectTo& connect_to) &;
  Request connect_to(const ConnectTo& connect_to) &&;

  template <typename T>
  std::enable_if_t<std::is_same_v<ConnectTo, T>, Request&> connect_to(T&&) {
    static_assert(
        !sizeof(T),
        "ConnectTo argument must not be temporary, it must outlive Request");
    return *this;
  }

  /// Override log URL. Usefull for "there's a secret in the query".
  /// @warning The query might be logged by other intermediate HTTP agents
  ///          (nginx, L7 balancer, etc.).
  Request& SetLoggedUrl(std::string url) &;
  Request SetLoggedUrl(std::string url) &&;

  /// Set destination name in metric "httpclient.destinations.<name>".
  /// If not set, defaults to HTTP path.  Should be called for all requests
  /// with parameters in HTTP path.
  Request& SetDestinationMetricName(const std::string& destination) &;
  Request SetDestinationMetricName(const std::string& destination) &&;

  /// @cond
  // Set testsuite related settings. For internal use only.
  Request& SetTestsuiteConfig(
      const std::shared_ptr<const TestsuiteConfig>& config) &;

  Request& SetAllowedUrlsExtra(const std::vector<std::string>& urls) &;

  // Set deadline propagation settings. For internal use only.
  Request& SetEnforceTaskDeadline(
      EnforceTaskDeadlineConfig enforce_task_deadline) &;

  Request& SetHeadersPropagator(const server::http::HeadersPropagator*) &;
  /// @endcond

  /// Disable auto-decoding of received replies.
  /// Useful to proxy replies 'as is'.
  Request& DisableReplyDecoding() &;
  Request DisableReplyDecoding() &&;

  Request& SetTracingManager(const tracing::TracingManagerBase&) &;
  Request SetTracingManager(const tracing::TracingManagerBase&) &&;

  /// Perform request asynchronously.
  ///
  /// Works well with engine::WaitAny, engine::WaitAnyFor, and
  /// engine::WaitUntil functions:
  /// @snippet src/clients/http/client_wait_test.cpp HTTP Client - waitany
  ///
  /// Request object could be reused after retrieval of data from
  /// ResponseFuture, all the setup holds:
  /// @snippet src/clients/http/client_test.cpp  HTTP Client - reuse async
  [[nodiscard]] ResponseFuture async_perform(
      utils::impl::SourceLocation location =
          utils::impl::SourceLocation::Current());

  /// @brief Perform a request with streamed response body.
  ///
  /// The HTTP client uses queue producer.
  /// StreamedResponse uses queue consumer.
  /// @see src/clients/http/partial_pesponse.hpp
  [[nodiscard]] StreamedResponse async_perform_stream_body(
      const std::shared_ptr<concurrent::StringStreamQueue>& queue,
      utils::impl::SourceLocation location =
          utils::impl::SourceLocation::Current());

  /// Calls async_perform and wait for timeout_ms on a future. Default time
  /// for waiting will be timeout value if it was set. If error occurred it
  /// will be thrown as exception.
  ///
  /// Request object could be reused after return from perform(), all the
  /// setup holds:
  /// @snippet src/clients/http/client_test.cpp  HTTP Client - request reuse
  [[nodiscard]] std::shared_ptr<Response> perform(
      utils::impl::SourceLocation location =
          utils::impl::SourceLocation::Current());

  /// Returns a reference to the original URL of a request
  const std::string& GetUrl() const&;
  const std::string& GetUrl() && = delete;

  /// Returns a reference to the HTTP body of a request to send
  const std::string& GetData() const&;
  const std::string& GetData() && = delete;

  /// Returns HTTP body of a request, leaving it empty
  std::string ExtractData();

 private:
  std::shared_ptr<RequestState> pimpl_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
