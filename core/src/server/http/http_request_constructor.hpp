#pragma once

#include <memory>

#include <userver/http/parser/http_request_parse_args.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_request_builder.hpp>
#include <userver/server/request/request_config.hpp>

#include "handler_info_index.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::http {

class HttpRequestConstructor final {
public:
    enum class Status {
        kOk,
        kBadRequest,
        kUriTooLong,
        kParseUrlError,
        kHandlerNotFound,
        kMethodNotAllowed,
        kHeadersTooLarge,
        kRequestTooLarge,
        kParseArgsError,
        kParseCookiesError,
        kParseMultipartFormDataError,
    };

    using Config = server::request::HttpRequestConfig;

    HttpRequestConstructor(
        Config config,
        const HandlerInfoIndex& handler_info_index,
        request::ResponseDataAccounter& data_accounter,
        engine::io::Sockaddr remote_address
    );

    ~HttpRequestConstructor();

    HttpRequestConstructor(HttpRequestConstructor&&) = delete;
    HttpRequestConstructor& operator=(HttpRequestConstructor&&) = delete;

    void SetMethod(HttpMethod method);
    void SetHttpMajor(unsigned short http_major);
    void SetHttpMinor(unsigned short http_minor);

    void AppendUrl(const char* data, size_t size);
    void ParseUrl();
    void AppendHeaderField(const char* data, size_t size);
    void AppendHeaderValue(const char* data, size_t size);
    void AppendBody(const char* data, size_t size);

    void SetIsFinal(bool is_final);

    // HTTP/2.0 only:
    void SetStreamProducer(impl::Http2StreamEventProducer&& producer);
    void SetResponseStreamId(std::int32_t stream_id);

    std::shared_ptr<http::HttpRequest> Finalize();

private:
    struct HttpParserUrl;

    void FinalizeImpl();

    void ParseArgs(const HttpParserUrl& url);
    void ParseArgs(const char* data, size_t size);
    void AddHeader();

    void SetStatus(Status status);
    void AccountRequestSize(size_t size);
    void AccountUrlSize(size_t size);
    void AccountHeadersSize(size_t size);

    void CheckStatus();

    Config config_;
    const HandlerInfoIndex& handler_info_index_;

    utils::FastPimpl<HttpParserUrl, 60, 8> parsed_url_pimpl_;
    std::string header_field_;
    std::string header_value_;
    bool header_field_flag_ = false;
    bool header_value_flag_ = false;

    size_t request_size_ = 0;
    size_t url_size_ = 0;
    size_t headers_size_ = 0;
    bool url_parsed_ = false;
    Status status_ = Status::kOk;

    std::string url_;
    std::string body_;
    HttpRequestBuilder builder_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
