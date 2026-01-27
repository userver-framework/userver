#pragma once

/// @file userver/clients/http/request.hpp
/// @brief @copybrief clients::http::Request

#include <memory>
#include <string_view>
#include <vector>

#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/clients/http/error.hpp>
#include <userver/clients/http/response.hpp>
#include <userver/clients/http/response_future.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/crypto/certificate.hpp>
#include <userver/crypto/private_key.hpp>
#include <userver/http/http_version.hpp>
#include <userver/utils/impl/internal_tag_fwd.hpp>
#include <userver/utils/impl/source_location.hpp>
#include <userver/utils/not_null.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {
class TracingManagerBase;
}  // namespace tracing

namespace utils::impl {
class WaitTokenStorageLock;
}  // namespace utils::impl

/// HTTP client helpers
namespace clients::http {

class RequestState;
class StreamedResponse;
class ConnectTo;
class Form;
struct DeadlinePropagationConfig;
class RequestStats;
class DestinationStatistics;
struct TestsuiteConfig;
class MiddlewareBase;

namespace impl {
class EasyWrapper;
}  // namespace impl

/// @brief HTTP request method
enum class HttpMethod { kGet, kPost, kHead, kPut, kDelete, kPatch, kOptions };

/// @brief Convert HTTP method enum value to string
std::string_view ToStringView(HttpMethod method);

/// @brief Convert HTTP method string to enum value
HttpMethod HttpMethodFromString(std::string_view method_str);

using USERVER_NAMESPACE::http::HttpVersion;

/// HTTP Authorization types
enum class HttpAuthType {
    kBasic,      ///< "basic"
    kDigest,     ///< "digest"
    kDigestIE,   ///< "digest_ie"
    kNegotiate,  ///< "negotiate"
    kNtlm,       ///< "ntlm"
    kNtlmWb,     ///< "ntlm_wb"
    kAny,        ///< "any"
    kAnySafe,    ///< "any_safe"
};

/// HTTP Proxy Authorization types
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

ProxyAuthType ProxyAuthTypeFromString(std::string_view auth_name);

/// @brief Class for creating and performing new http requests, usually retieved from @ref clients::http::Client.
class Request final {
public:
    /// Request cookies container type
    using Cookies = std::unordered_map<std::string, std::string, utils::StrCaseHash>;

    /// @cond
    // For internal use only.
    explicit Request(
        impl::EasyWrapper&&,
        RequestStats&& req_stats,
        const std::shared_ptr<DestinationStatistics>& dest_stats,
        clients::dns::Resolver* resolver,
        const tracing::TracingManagerBase& tracing_manager
    );
    /// @endcond

    /// Specifies method
    Request& method(HttpMethod method) &;
    /// @overload
    Request method(HttpMethod method) &&;

    /// GET request
    Request& get() &;
    /// @overload
    Request get() &&;
    /// @overload
    Request& get(std::string url) &;
    /// @overload
    Request get(std::string url) &&;

    /// HEAD request
    Request& head() &;
    /// @overload
    Request head() &&;
    /// @overload
    Request& head(std::string url) &;
    /// @overload
    Request head(std::string url) &&;

    /// POST request
    Request& post() &;
    /// @overload
    Request post() &&;
    /// @overload
    Request& post(std::string url, std::string data = {}) &;
    /// @overload
    Request post(std::string url, std::string data = {}) &&;
    /// @overload
    Request& post(std::string url, Form&& form) &;
    /// @overload
    Request post(std::string url, Form&& form) &&;

    /// PUT request
    Request& put() &;
    /// @overload
    Request put() &&;
    /// @overload
    Request& put(std::string url, std::string data = {}) &;
    /// @overload
    Request put(std::string url, std::string data = {}) &&;

    /// PATCH request
    Request& patch() &;
    /// @overload
    Request patch() &&;
    /// @overload
    Request& patch(std::string url, std::string data = {}) &;
    /// @overload
    Request patch(std::string url, std::string data = {}) &&;

    /// DELETE request
    Request& delete_method() &;
    /// @overload
    Request delete_method() &&;
    /// @overload
    Request& delete_method(std::string url) &;
    /// @overload
    Request delete_method(std::string url) &&;
    /// @overload
    Request& delete_method(std::string url, std::string data) &;
    /// @overload
    Request delete_method(std::string url, std::string data) &&;

    /// Set custom request method. Only replaces name of the HTTP method
    Request& set_custom_http_request_method(std::string method) &;
    /// @overload
    Request set_custom_http_request_method(std::string method) &&;

    /// url if you don't specify request type with url
    Request& url(std::string url) &;
    /// @overload
    Request url(std::string url) &&;

    /// Set data for POST request
    Request& data(std::string data) &;
    /// @overload
    Request data(std::string data) &&;

    /// Set 'form' for POST request
    Request& form(Form&& form) &;
    /// @overload
    Request form(Form&& form) &&;

    /// Set headers for request
    Request& headers(const Headers& headers) &;
    /// @overload
    Request headers(const Headers& headers) &&;
    /// @overload
    Request& headers(const std::initializer_list<std::pair<utils::zstring_view, utils::zstring_view>>& headers) &;
    /// @overload
    Request headers(const std::initializer_list<std::pair<utils::zstring_view, utils::zstring_view>>& headers) &&;

    /// Sets http auth type to use.
    Request& http_auth_type(
        HttpAuthType value,
        bool auth_only,
        utils::zstring_view user,
        utils::zstring_view password
    ) &;
    /// @overload
    Request http_auth_type(
        HttpAuthType value,
        bool auth_only,
        utils::zstring_view user,
        utils::zstring_view password
    ) &&;

    /// Set proxy headers for request
    Request& proxy_headers(const Headers& headers) &;
    /// @overload
    Request proxy_headers(const Headers& headers) &&;
    /// @overload
    Request& proxy_headers(const std::initializer_list<std::pair<utils::zstring_view, utils::zstring_view>>& headers) &;
    /// @overload
    Request proxy_headers(const std::initializer_list<std::pair<utils::zstring_view, utils::zstring_view>>& headers) &&;

    /// Sets the User-Agent header
    Request& user_agent(utils::zstring_view value) &;
    /// @overload
    Request user_agent(utils::zstring_view value) &&;

    /// Sets proxy to use.
    ///
    /// Example: [::1]:1080
    /// Empty string disables proxy.
    Request& proxy(utils::zstring_view value) &;
    /// @overload
    Request proxy(utils::zstring_view value) &&;

    /// Sets proxy auth type to use.
    Request& proxy_auth_type(ProxyAuthType value) &;
    /// @overload
    Request proxy_auth_type(ProxyAuthType value) &&;

    /// Cookies for request as HashDos-safe map
    Request& cookies(const Cookies& cookies) &;
    /// Cookies for request as HashDos-safe map
    Request cookies(const Cookies& cookies) &&;

    /// Cookies for request as map
    Request& cookies(const std::unordered_map<std::string, std::string>& cookies) &;
    /// Cookies for request as map
    Request cookies(const std::unordered_map<std::string, std::string>& cookies) &&;

    /// Follow redirects or not. Default: follow
    Request& follow_redirects(bool follow = true) &;
    /// @overload
    Request follow_redirects(bool follow = true) &&;

    /// The maximum time in milliseconds for the entire transfer operation (from name lookup and connection
    /// construction to the end of data acquisition).
    Request& timeout(long timeout_ms) &;
    /// @overload
    Request timeout(long timeout_ms) &&;
    /// @overload
    Request& timeout(std::chrono::milliseconds timeout_ms) & { return timeout(timeout_ms.count()); }
    /// @overload
    Request timeout(std::chrono::milliseconds timeout_ms) && { return std::move(this->timeout(timeout_ms.count())); }

    /// Verify host and peer or not. Default: verify
    Request& verify(bool verify = true) &;
    /// @overload
    Request verify(bool verify = true) &&;

    /// Set file holding one or more certificates to verify the peer with
    Request& ca_info(utils::zstring_view file_path) &;
    /// @overload
    Request ca_info(utils::zstring_view file_path) &&;

    /// Set CA
    Request& ca(crypto::Certificate cert) &;
    /// @overload
    Request ca(crypto::Certificate cert) &&;

    /// Set CRL-file
    Request& crl_file(utils::zstring_view file_path) &;
    /// @overload
    Request crl_file(utils::zstring_view file_path) &&;

    /// Set private client key and certificate for request.
    ///
    /// @warning Do not use this function on MacOS as it may cause Segmentation Fault on that platform.
    Request& client_key_cert(crypto::PrivateKey pkey, crypto::Certificate cert) &;
    /// @overload
    Request client_key_cert(crypto::PrivateKey pkey, crypto::Certificate cert) &&;

    /// Set HTTP version
    Request& http_version(HttpVersion version) &;
    /// @overload
    Request http_version(HttpVersion version) &&;

    /// Specify number of retries on incorrect status, if on_fails is True
    /// retry on network error too. Retries = 3 means that maximum 3 request will be performed.
    ///
    /// Retries use exponential backoff with jitter - an exponentially increasing
    /// randomized delay is added before each retry of this request.
    Request& retry(short retries = 3, bool on_fails = true) &;
    Request retry(short retries = 3, bool on_fails = true) &&;

    /// Set unix domain socket as connection endpoint and provide path to it
    /// When enabled, request will connect to the Unix domain socket instead
    /// of establishing a TCP connection to a host.
    Request& unix_socket_path(utils::zstring_view path) &;
    Request unix_socket_path(utils::zstring_view path) &&;

    /// Set CURL_IPRESOLVE_V4 for ipv4 resolving
    Request& use_ipv4() &;
    /// @overload
    Request use_ipv4() &&;

    /// Set CURL_IPRESOLVE_V6 for ipv6 resolving
    Request& use_ipv6() &;
    /// @overload
    Request use_ipv6() &&;

    /// Set CURLOPT_CONNECT_TO option
    /// @warning connect_to argument must outlive Request
    Request& connect_to(const ConnectTo& connect_to) &;
    /// @overload
    Request connect_to(const ConnectTo& connect_to) &&;

    /// @overload
    template <typename T>
    std::enable_if_t<std::is_same_v<ConnectTo, T>, Request&> connect_to(T&&) {
        static_assert(!sizeof(T), "ConnectTo argument must not be temporary, it must outlive Request");
        return *this;
    }

    /// Override list of middlewares from @ref components::HttpClient for specific request
    Request& SetMiddlewaresList(const std::vector<utils::NotNull<MiddlewareBase*>>& middlewares) &;

    /// Override log URL. Useful for "there's a secret in the query".
    /// @warning The query might be logged by other intermediate HTTP agents (nginx, L7 balancer, etc.).
    Request& SetLoggedUrl(std::string url) &;
    /// @overload
    Request SetLoggedUrl(std::string url) &&;

    /// Set URL template (low-cardinality) for tracing, i.e. `/v1/user/{user_id}`
    /// @see https://opentelemetry.io/docs/specs/semconv/registry/attributes/url/
    Request& SetUrlTemplate(std::string url_template) &;
    /// @overload
    Request SetUrlTemplate(std::string url_template) &&;

    /// Set destination name in metric "httpclient.destinations.<name>".
    /// If not set, defaults to HTTP path.  Should be called for all requests
    /// with parameters in HTTP path.
    Request& SetDestinationMetricName(const std::string& destination) &;
    /// @overload
    Request SetDestinationMetricName(const std::string& destination) &&;

    /// @cond
    // Set testsuite related settings. For internal use only.
    void SetTestsuiteConfig(const std::shared_ptr<const TestsuiteConfig>& config) &;

    void SetAllowedUrlsExtra(const std::vector<std::string>& urls) &;

    // Set deadline propagation settings. For internal use only.
    void SetDeadlinePropagationConfig(const DeadlinePropagationConfig& deadline_propagation_config) &;

    void SetWaitToken(utils::impl::InternalTag, utils::impl::WaitTokenStorageLock&&);
    /// @endcond

    /// Disable auto-decoding of received replies. Useful to proxy replies 'as is'.
    Request& DisableReplyDecoding() &;
    /// @overload
    Request DisableReplyDecoding() &&;

    void SetCancellationPolicy(CancellationPolicy cp);

    /// Override the default tracing manager from HTTP client for this particular request.
    Request& SetTracingManager(const tracing::TracingManagerBase&) &;
    /// @overload
    Request SetTracingManager(const tracing::TracingManagerBase&) &&;

    /// Perform request asynchronously.
    ///
    /// Works well with @ref engine::WaitAny(), @ref engine::WaitAnyFor(), and @ref engine::WaitUntil() functions:
    /// @snippet src/clients/http/client_wait_test.cpp HTTP Client - waitany
    ///
    /// Refrain from reusing the Request object.
    /// Though it might be possible to reuse it after extracting data from ResponseFuture, a subsequent async_perform
    /// or perform call could be delayed until the previous request fully completes. This delay can occur if the
    /// previous request either timed out or was canceled.
    /// Future versions might entirely forbid Request objects reuse.
    [[nodiscard]] ResponseFuture async_perform(
        utils::impl::SourceLocation location = utils::impl::SourceLocation::Current()
    );

    /// @brief Perform a request with streamed response body.
    ///
    /// The HTTP client uses queue producer. StreamedResponse uses queue consumer.
    [[nodiscard]] StreamedResponse async_perform_stream_body(
        const std::shared_ptr<concurrent::StringStreamQueue>& queue,
        utils::impl::SourceLocation location = utils::impl::SourceLocation::Current()
    );

    /// Calls async_perform and wait for timeout_ms on a future. Default time  for waiting will be timeout value if it
    /// was set. If error occurred it will be thrown as exception.
    ///
    /// Refrain from reusing the Request object.
    /// Though it might be possible to reuse it after extracting data from ResponseFuture, a subsequent async_perform
    /// or perform call could be delayed until the previous request fully completes. This delay can occur if the
    /// previous request either timed out or was canceled.
    /// Future versions might entirely forbid Request objects reuse.
    [[nodiscard]] std::shared_ptr<Response> perform(
        utils::impl::SourceLocation location = utils::impl::SourceLocation::Current()
    );

    /// Returns a reference to the original URL of a request
    const std::string& GetUrl() const&;
    /// @overload
    const std::string& GetUrl() && = delete;

    /// Returns a reference to the HTTP body of a request to send
    const std::string& GetData() const&;
    /// @overload
    const std::string& GetData() && = delete;

    /// Returns HTTP body of a request, leaving it empty
    std::string ExtractData();

private:
    std::shared_ptr<RequestState> pimpl_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
