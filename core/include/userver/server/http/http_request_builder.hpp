#pragma once

/// @file userver/server/http/http_request_builder.hpp
/// @brief @copybrief server::http::HttpRequestBuilder

#include <memory>

#include <userver/server/http/http_request.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

/// @brief HTTP Request Builder
class HttpRequestBuilder final {
public:
    /// @cond
    explicit HttpRequestBuilder(request::ResponseDataAccounter& data_accounter);
    /// @endcond

    HttpRequestBuilder();

    explicit HttpRequestBuilder(const HttpRequest& data_accounter);

    HttpRequestBuilder& SetRemoteAddress(engine::io::Sockaddr remote_address);

    HttpRequestBuilder& SetMethod(HttpMethod method);

    HttpRequestBuilder& SetHttpMajor(int http_major);

    HttpRequestBuilder& SetHttpMinor(int http_minor);

    HttpRequestBuilder& SetBody(std::string&& body);

    HttpRequestBuilder& AddHeader(std::string&& header, std::string&& value);

    HttpRequestBuilder& AddRequestArg(std::string&& key, std::string&& value);

    HttpRequestBuilder& SetPathArgs(std::vector<std::pair<std::string, std::string>> args);

    HttpRequestBuilder& SetUrl(std::string&& url);

    HttpRequestBuilder& SetRequestPath(std::string&& path);

    std::shared_ptr<HttpRequest> Build();

    /// @cond
    HttpRequestBuilder& SetIsFinal(bool is_final);

    HttpRequestBuilder& SetFormDataArgs(
        utils::impl::TransparentMap<std::string, std::vector<FormDataArg>, utils::StrCaseHash>&& form_data_args
    );

    HttpRequestBuilder& SetHttpHandler(const handlers::HttpHandlerBase& handler);

    HttpRequestBuilder& SetTaskProcessor(engine::TaskProcessor& task_processor);

    HttpRequestBuilder& SetHttpHandlerStatistics(handlers::HttpRequestStatistics& stats);

    // TODO: remove?
    HttpRequestBuilder& SetStreamProducer(impl::Http2StreamEventProducer&& producer);

    // TODO: remove?
    HttpRequestBuilder& SetResponseStreamId(std::int32_t stream_id);

    HttpRequestBuilder& SetResponseStatus(HttpStatus status);

    const HttpRequest& GetRef() const;

    HttpResponse& GetHttpResponse();
    /// @endcond

private:
    void ParseCookies();

    std::shared_ptr<HttpRequest> request_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
