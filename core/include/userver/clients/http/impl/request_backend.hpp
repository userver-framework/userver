#pragma once

/// @file userver/clients/http/impl/request_backend.hpp
/// @brief @copybrief clients::http::impl::RequestBackend

#include <chrono>
#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <userver/clients/http/request.hpp>
#include <userver/utils/impl/source_location.hpp>
#include <userver/utils/zstring_view.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http::impl {

/// @brief Extension point for HTTP requests created by an alternative client.
///
/// Construct @ref clients::http::Request with a shared backend from
/// @ref clients::http::Client::CreateRequest. Copies of @ref clients::http::Request share this backend;
/// concurrent mutation or execution of the same request is not supported.
/// Setters must retain owned inputs before returning; borrowed dependencies follow
/// the lifetime requirements of the corresponding @ref clients::http::Request operation. An in-flight operation must
/// keep its request data and transport dependencies alive after @ref clients::http::Request is destroyed.
///
/// Implementations provide the semantics of the corresponding @ref clients::http::Request
/// operations and explicitly reject features they do not support. Map request failures
/// to the @ref clients::http::BaseException hierarchy.
class RequestBackend {
public:
    /// @brief Retry settings for one logical request.
    struct RetryPolicy {
        /// Maximum number of attempts, including the initial attempt; always positive.
        unsigned short max_attempts{1};
        /// Whether transport failures, in addition to retryable HTTP responses, may be retried.
        bool retry_on_network_errors{false};
    };

    virtual ~RequestBackend();

    /// @brief Sets the HTTP method using the semantics of @ref clients::http::Request::method.
    virtual void SetMethod(HttpMethod method) = 0;

    /// @brief Sets and validates the URL; invalid URLs throw @ref clients::http::BadArgumentException.
    virtual void SetUrl(std::string url) = 0;

    /// @brief Appends a header, retaining previous occurrences; User-Agent replaces its previous value.
    /// Empty values are sent as empty headers.
    virtual void AddHeader(utils::zstring_view name, utils::zstring_view value) = 0;

    /// @brief Sets the buffered request body, including an explicitly empty body.
    virtual void SetBody(std::string body) = 0;

    /// @brief Sets the timeout for a complete attempt, including connection setup and transfer.
    virtual void SetTimeout(std::chrono::milliseconds timeout) = 0;

    /// @brief Sets the attempt limit and transport-failure retry policy.
    /// The backend owns retry selection and backoff, and must bound waiting accordingly.
    virtual void SetRetryPolicy(RetryPolicy policy) = 0;

    /// @brief Returns the original URL; the reference remains valid until the next URL mutation.
    virtual const std::string& GetUrl() const = 0;

    /// @brief Returns the buffered body; the reference remains valid until its next mutation or extraction.
    virtual const std::string& GetBody() const = 0;

    /// @brief Extracts the buffered body, leaving it empty.
    virtual std::string ExtractBody() = 0;

    /// @brief Starts the request without waiting for its response.
    /// The returned future owns completion and cancellation independently of this builder.
    virtual ResponseFuture AsyncPerform(utils::impl::SourceLocation location) = 0;

    /// @brief Performs the request synchronously using the semantics of @ref clients::http::Request::perform.
    virtual std::shared_ptr<Response> Perform(utils::impl::SourceLocation location) = 0;

    /// @brief Implements @ref clients::http::Request::async_perform_stream_body.
    virtual StreamedResponse AsyncPerformStreamBody(
        const std::shared_ptr<concurrent::StringStreamQueue>& queue,
        utils::impl::SourceLocation location
    ) = 0;

    /// @brief Implements @ref clients::http::Request::PerformWebSocketHandshake.
    virtual WebSocketResponse PerformWebSocketHandshake(utils::impl::SourceLocation location) = 0;

    /// @brief Implements @ref clients::http::Request::follow_redirects.
    virtual void SetFollowRedirects(bool follow) = 0;

    /// @brief Implements @ref clients::http::Request::verify.
    virtual void SetVerify(bool verify) = 0;

    /// @brief Implements @ref clients::http::Request::ca_info.
    virtual void SetCaInfo(utils::zstring_view file_path) = 0;

    /// @brief Implements @ref clients::http::Request::ca.
    virtual void SetCa(crypto::Certificate cert) = 0;

    /// @brief Implements @ref clients::http::Request::crl_file.
    virtual void SetCrlFile(utils::zstring_view file_path) = 0;

    /// @brief Implements @ref clients::http::Request::client_key_cert.
    virtual void SetClientKeyCert(crypto::PrivateKey pkey, crypto::Certificate cert) = 0;

    /// @brief Implements @ref clients::http::Request::http_version.
    virtual void SetHttpVersion(HttpVersion version) = 0;

    /// @brief Implements @ref clients::http::Request::unix_socket_path.
    virtual void SetUnixSocketPath(utils::zstring_view path) = 0;

    /// @brief Implements @ref clients::http::Request::use_ipv4.
    virtual void UseIpv4() = 0;

    /// @brief Implements @ref clients::http::Request::use_ipv6.
    virtual void UseIpv6() = 0;

    /// @brief Implements @ref clients::http::Request::connect_to.
    virtual void SetConnectTo(const ConnectTo& connect_to) = 0;

    /// @brief Implements @ref clients::http::Request::form.
    virtual void SetForm(Form&& form) = 0;

    /// @brief Implements @ref clients::http::Request::http_auth_type.
    virtual void SetHttpAuthType(
        HttpAuthType value,
        bool auth_only,
        utils::zstring_view user,
        utils::zstring_view password
    ) = 0;

    /// @brief Implements @ref clients::http::Request::proxy_headers.
    virtual void SetProxyHeaders(const Headers& headers) = 0;

    /// @brief Implements @ref clients::http::Request::proxy_headers.
    virtual void SetProxyHeaders(const std::initializer_list<
                                 std::pair<utils::zstring_view, utils::zstring_view>>& headers) = 0;

    /// @brief Implements @ref clients::http::Request::user_agent.
    virtual void SetUserAgent(utils::zstring_view value) = 0;

    /// @brief Implements @ref clients::http::Request::proxy.
    virtual void SetProxy(utils::zstring_view value) = 0;

    /// @brief Implements @ref clients::http::Request::proxy_auth_type.
    virtual void SetProxyAuthType(ProxyAuthType value) = 0;

    /// @brief Implements @ref clients::http::Request::cookies.
    virtual void SetCookies(const Request::Cookies& cookies) = 0;

    /// @brief Implements @ref clients::http::Request::cookies.
    virtual void SetCookies(const std::unordered_map<std::string, std::string>& cookies) = 0;

    /// @brief Implements @ref clients::http::Request::set_custom_http_request_method.
    virtual void SetCustomHttpRequestMethod(std::string method) = 0;

    /// @brief Implements @ref clients::http::Request::SetMiddlewaresList.
    virtual void SetMiddlewaresList(const std::vector<utils::NotNull<MiddlewareBase*>>& middlewares) = 0;

    /// @brief Implements @ref clients::http::Request::SetIncompleteTlsConnectionCloseExpected.
    virtual void SetIncompleteTlsConnectionCloseExpected(bool expect) = 0;

    /// @brief Implements @ref clients::http::Request::SetLoggedUrl.
    virtual void SetLoggedUrl(std::string url) = 0;

    /// @brief Implements @ref clients::http::Request::SetExpectedStatusCodes.
    virtual void SetExpectedStatusCodes(ExpectedStatusCodes status_codes) = 0;

    /// @brief Implements @ref clients::http::Request::SetUrlTemplate.
    virtual void SetUrlTemplate(std::string url_template) = 0;

    /// @brief Implements @ref clients::http::Request::SetDestinationMetricName.
    virtual void SetDestinationMetricName(const std::string& destination) = 0;

    /// @brief Sets the testsuite configuration for the request.
    virtual void SetTestsuiteConfig(const std::shared_ptr<const TestsuiteConfig>& config) = 0;

    /// @brief Sets additional URLs allowed by testsuite restrictions.
    virtual void SetAllowedUrlsExtra(const std::vector<std::string>& urls) = 0;

    /// @brief Sets deadline propagation settings for the request.
    virtual void SetDeadlinePropagationConfig(const DeadlinePropagationConfig& deadline_propagation_config) = 0;

    /// @brief Implements @ref clients::http::Request::DisableReplyDecoding.
    virtual void DisableReplyDecoding() = 0;

    /// @brief Implements @ref clients::http::Request::SetCancellationPolicy.
    virtual void SetCancellationPolicy(CancellationPolicy cp) = 0;

    /// @brief Implements @ref clients::http::Request::SetTracingManager.
    virtual void SetTracingManager(const tracing::TracingManagerBase& tracing_manager) = 0;

    /// @brief Implements @ref clients::http::Request::SetUrlUserinfoAllowed.
    virtual void SetUrlUserinfoAllowed(bool allowed) = 0;

    /// @brief Takes ownership of the internal wait token for the request.
    virtual void SetWaitToken(utils::impl::WaitTokenStorageLock&& wait_token) = 0;
};

}  // namespace clients::http::impl

USERVER_NAMESPACE_END
