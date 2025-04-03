#pragma once

/// @file userver/server/http/http_request.hpp
/// @brief @copybrief server::http::HttpRequest

#include <chrono>
#include <string>
#include <vector>

#include <userver/engine/task/task_processor_fwd.hpp>

#include <userver/engine/io/sockaddr.hpp>
#include <userver/server/http/form_data_arg.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/utils/datetime/wall_coarse_clock.hpp>
#include <userver/utils/impl/internal_tag.hpp>
#include <userver/utils/impl/transparent_hash.hpp>
#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {
class HttpRequestStatistics;
class HttpHandlerBase;
}  // namespace server::handlers

/// Server parts of the HTTP protocol implementation.
namespace server::http {

/// @brief HTTP Request data.
/// @note do not create HttpRequest by hand in tests,
///       use HttpRequestBuilder instead.
class HttpRequest final {
public:
    using HeadersMap = USERVER_NAMESPACE::http::headers::HeaderMap;

    using HeadersMapKeys = decltype(utils::impl::MakeKeysView(HeadersMap()));

    using CookiesMap = std::unordered_map<std::string, std::string, utils::StrCaseHash>;

    using CookiesMapKeys = decltype(utils::impl::MakeKeysView(CookiesMap()));

    /// @cond
    explicit HttpRequest(request::ResponseDataAccounter& data_accounter, utils::impl::InternalTag);
    /// @endcond

    HttpRequest(HttpRequest&&) = delete;
    HttpRequest(const HttpRequest&) = delete;

    ~HttpRequest();

    /// @return HTTP method (e.g. GET/POST)
    const HttpMethod& GetMethod() const;

    /// @return HTTP method as a string (e.g. "GET")
    const std::string& GetMethodStr() const;

    /// @return Major version of HTTP. For example, for HTTP 1.0 it returns 1
    int GetHttpMajor() const;

    /// @return Minor version of HTTP. For example, for HTTP 1.0 it returns 0
    int GetHttpMinor() const;

    /// @return Request URL
    const std::string& GetUrl() const;

    /// @return Request path
    const std::string& GetRequestPath() const;

    /// @cond
    std::chrono::duration<double> GetRequestTime() const;

    std::chrono::duration<double> GetResponseTime() const;
    /// @endcond

    /// @return Host from the URL.
    const std::string& GetHost() const;

    /// @return Request remote (peer's) address
    const engine::io::Sockaddr& GetRemoteAddress() const;

    /// @return First argument value with name `arg_name` or an empty string if no
    /// such argument.
    /// Arguments are extracted from:
    /// - query part of the URL,
    /// - the HTTP body (only if `parse_args_from_body: true` for handler is set).
    ///
    /// In both cases, arg keys and values are url-decoded automatically when
    /// parsing into the HttpRequest.
    const std::string& GetArg(std::string_view arg_name) const;

    /// @return Argument values with name `arg_name` or an empty vector if no
    /// such argument.
    /// Arguments are extracted from:
    /// - query part of the URL,
    /// - the HTTP body (only if `parse_args_from_body: true` for handler is set).
    ///
    /// In both cases, arg keys and values are url-decoded automatically when
    /// parsing into the HttpRequest.
    const std::vector<std::string>& GetArgVector(std::string_view arg_name) const;

    /// @return true if argument with name arg_name exists, false otherwise.
    /// Arguments are extracted from:
    /// - query part of the URL,
    /// - the HTTP body (only if `parse_args_from_body: true` for handler is set).
    ///
    /// In both cases, arg keys and values are url-decoded automatically when
    /// parsing into the HttpRequest.
    bool HasArg(std::string_view arg_name) const;

    /// @return Count of arguments.
    /// Arguments are extracted from:
    /// - query part of the URL,
    /// - the HTTP body (only if `parse_args_from_body: true` for handler is set).
    size_t ArgCount() const;

    /// @return List of names of arguments.
    /// Arguments are extracted from:
    /// - query part of the URL,
    /// - the HTTP body (only if `parse_args_from_body: true` for handler is set).
    std::vector<std::string> ArgNames() const;

    /// @return First argument value with name arg_name from multipart/form-data
    /// request or an empty FormDataArg if no such argument.
    const FormDataArg& GetFormDataArg(std::string_view arg_name) const;

    /// @return Argument values with name arg_name from multipart/form-data
    /// request or an empty FormDataArg if no such argument.
    const std::vector<FormDataArg>& GetFormDataArgVector(std::string_view arg_name) const;

    /// @return true if argument with name arg_name exists in multipart/form-data
    /// request, false otherwise.
    bool HasFormDataArg(std::string_view arg_name) const;

    /// @return Count of multipart/form-data arguments.
    size_t FormDataArgCount() const;

    /// @return List of names of multipart/form-data arguments.
    std::vector<std::string> FormDataArgNames() const;

    /// @return Named argument from URL path with wildcards.
    /// @note Path args are currently NOT url-decoded automatically.
    const std::string& GetPathArg(std::string_view arg_name) const;

    /// @return Argument from URL path with wildcards by its 0-based index.
    /// @note Path args are currently NOT url-decoded automatically.
    const std::string& GetPathArg(size_t index) const;

    /// @return true if named argument from URL path with wildcards exists, false
    /// otherwise.
    bool HasPathArg(std::string_view arg_name) const;

    /// @return true if argument with index from URL path with wildcards exists,
    /// false otherwise.
    bool HasPathArg(size_t index) const;

    /// @return Number of wildcard arguments in URL path.
    size_t PathArgCount() const;

    /// @return Value of the header with case insensitive name header_name, or an
    /// empty string if no such header.
    const std::string& GetHeader(std::string_view header_name) const;

    /// @overload
    const std::string& GetHeader(const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name) const;

    /// @return true if header with case insensitive name header_name exists,
    /// false otherwise.
    bool HasHeader(std::string_view header_name) const;

    /// @overload
    bool HasHeader(const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name) const;

    /// @return Number of headers.
    size_t HeaderCount() const;

    /// Removes the header with case insensitive name header_name.
    void RemoveHeader(std::string_view header_name);

    /// @overload
    void RemoveHeader(const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name);

    /// @return List of headers names.
    HeadersMapKeys GetHeaderNames() const;

    /// @return HTTP headers.
    const HeadersMap& GetHeaders() const;

    /// @return Value of the cookie with case sensitive name cookie_name, or an
    /// empty string if no such cookie exists.
    const std::string& GetCookie(const std::string& cookie_name) const;

    /// @return true if cookie with case sensitive name cookie_name exists, false
    /// otherwise.
    bool HasCookie(const std::string& cookie_name) const;

    /// @return Number of cookies.
    size_t CookieCount() const;

    /// @return List of cookies names.
    CookiesMapKeys GetCookieNames() const;

    /// @return HTTP cookies.
    const CookiesMap& RequestCookies() const;

    /// @return HTTP body.
    const std::string& RequestBody() const;

    /// @return moved out HTTP body. `this` is modified.
    std::string ExtractRequestBody();

    /// @cond
    void SetRequestBody(std::string body);
    void ParseArgsFromBody();
    bool IsFinal() const;
    /// @endcond

    /// @brief Set the response status code.
    ///
    /// Equivalent to this->GetHttpResponse().SetStatus(status).
    void SetResponseStatus(HttpStatus status) const;

    /// @return true if the body of the request is still compressed. In other
    /// words returns true if the static option `decompress_request` of a handler
    /// was set to `false` and this is a compressed request.
    bool IsBodyCompressed() const;

    HttpResponse& GetHttpResponse() const;

    /// Get approximate time point of request handling start
    std::chrono::steady_clock::time_point GetStartTime() const;

    /// @cond
    void MarkAsInternalServerError() const;

    void SetStartSendResponseTime();
    void SetFinishSendResponseTime();

    void WriteAccessLogs(
        const logging::TextLoggerPtr& logger_access,
        const logging::TextLoggerPtr& logger_access_tskv,
        const std::string& remote_address
    ) const;

    void WriteAccessLog(
        const logging::TextLoggerPtr& logger_access,
        utils::datetime::WallCoarseClock::time_point tp,
        const std::string& remote_address
    ) const;

    void WriteAccessTskvLog(
        const logging::TextLoggerPtr& logger_access_tskv,
        utils::datetime::WallCoarseClock::time_point tp,
        const std::string& remote_address
    ) const;

    using UpgradeCallback = std::function<void(std::unique_ptr<engine::io::RwBase>&&, engine::io::Sockaddr&&)>;

    bool IsUpgradeWebsocket() const;
    void SetUpgradeWebsocket(UpgradeCallback cb) const;
    void DoUpgrade(std::unique_ptr<engine::io::RwBase>&& socket, engine::io::Sockaddr&& peer_name) const;
    /// @endcond

private:
    void AccountResponseTime();

    void SetPathArgs(std::vector<std::pair<std::string, std::string>> args);

    void SetHttpHandler(const handlers::HttpHandlerBase& handler);
    const handlers::HttpHandlerBase* GetHttpHandler() const;

    void SetTaskProcessor(engine::TaskProcessor& task_processor);
    engine::TaskProcessor* GetTaskProcessor() const;

    void SetHttpHandlerStatistics(handlers::HttpRequestStatistics&);

    // HTTP/2.0 only
    void SetResponseStreamId(std::int32_t);
    void SetStreamProducer(impl::Http2StreamEventProducer&& producer);

    void SetTaskCreateTime();
    void SetTaskStartTime();
    void SetResponseNotifyTime();
    void SetResponseNotifyTime(std::chrono::steady_clock::time_point now);

    friend class HttpRequestBuilder;
    friend class HttpRequestHandler;

    struct Impl;
    utils::FastPimpl<Impl, 1648, 16> pimpl_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
