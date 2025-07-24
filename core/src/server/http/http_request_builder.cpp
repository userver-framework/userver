#include <userver/server/http/http_request_builder.hpp>

#include <server/http/http_request_impl.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

inline void Strip(const char*& begin, const char*& end) {
    while (begin < end && isspace(*begin)) ++begin;
    while (begin < end && isspace(end[-1])) --end;
}

request::ResponseDataAccounter default_data_accounter;

}  // namespace

HttpRequestBuilder::HttpRequestBuilder(request::ResponseDataAccounter& data_accounter)
    : request_(std::make_shared<HttpRequest>(data_accounter, utils::impl::InternalTag{})) {}

HttpRequestBuilder::HttpRequestBuilder() : HttpRequestBuilder(default_data_accounter) {}

HttpRequestBuilder& HttpRequestBuilder::SetRemoteAddress(engine::io::Sockaddr remote_address) {
    request_->pimpl_->remote_address_ = std::move(remote_address);
    return *this;
}

HttpRequestBuilder& HttpRequestBuilder::SetMethod(HttpMethod method) {
    request_->pimpl_->method_ = method;
    return *this;
}

HttpRequestBuilder& HttpRequestBuilder::SetHttpMajor(int http_major) {
    request_->pimpl_->http_major_ = http_major;
    return *this;
}

HttpRequestBuilder& HttpRequestBuilder::SetHttpMinor(int http_minor) {
    request_->pimpl_->http_minor_ = http_minor;
    return *this;
}

HttpRequestBuilder& HttpRequestBuilder::SetBody(std::string&& body) {
    request_->SetRequestBody(std::move(body));
    return *this;
}

HttpRequestBuilder& HttpRequestBuilder::AddHeader(std::string&& header, std::string&& value) {
    request_->pimpl_->headers_.InsertOrAppend(std::move(header), std::move(value));
    return *this;
}

HttpRequestBuilder& HttpRequestBuilder::AddRequestArg(std::string&& key, std::string&& value) {
    request_->pimpl_->request_args_[std::move(key)].push_back(std::move(value));
    return *this;
}

HttpRequestBuilder& HttpRequestBuilder::SetPathArgs(std::vector<std::pair<std::string, std::string>> args) {
    request_->SetPathArgs(std::move(args));
    return *this;
}

HttpRequestBuilder& HttpRequestBuilder::SetUrl(std::string&& url) {
    request_->pimpl_->url_ = std::move(url);
    return *this;
}

HttpRequestBuilder& HttpRequestBuilder::SetRequestPath(std::string&& path) {
    request_->pimpl_->request_path_ = std::move(path);
    return *this;
}

HttpRequestBuilder& HttpRequestBuilder::SetIsFinal(bool is_final) {
    request_->pimpl_->is_final_ = is_final;
    return *this;
}

HttpRequestBuilder& HttpRequestBuilder::SetFormDataArgs(
    utils::impl::TransparentMap<std::string, std::vector<FormDataArg>, utils::StrCaseHash>&& form_data_args
) {
    request_->pimpl_->form_data_args_ = std::move(form_data_args);
    return *this;
}

HttpRequestBuilder& HttpRequestBuilder::SetHttpHandler(const handlers::HttpHandlerBase& handler) {
    request_->SetHttpHandler(handler);
    return *this;
}

HttpRequestBuilder& HttpRequestBuilder::SetTaskProcessor(engine::TaskProcessor& task_processor) {
    request_->SetTaskProcessor(task_processor);
    return *this;
}

HttpRequestBuilder& HttpRequestBuilder::SetHttpHandlerStatistics(handlers::HttpRequestStatistics& stats) {
    request_->SetHttpHandlerStatistics(stats);
    return *this;
}

HttpRequestBuilder& HttpRequestBuilder::SetStreamProducer(impl::Http2StreamEventProducer&& producer) {
    request_->SetStreamProducer(std::move(producer));
    return *this;
}

HttpRequestBuilder& HttpRequestBuilder::SetResponseStreamId(std::int32_t stream_id) {
    request_->SetResponseStreamId(stream_id);
    return *this;
}

HttpRequestBuilder& HttpRequestBuilder::SetResponseStatus(HttpStatus status) {
    request_->SetResponseStatus(status);
    return *this;
}

const HttpRequest& HttpRequestBuilder::GetRef() const {
    UASSERT(request_);
    return *request_;
}

HttpResponse& HttpRequestBuilder::GetHttpResponse() { return request_->GetHttpResponse(); }

std::shared_ptr<HttpRequest> HttpRequestBuilder::Build() {
    ParseCookies();

    UASSERT(std::all_of(
        request_->pimpl_->request_args_.begin(),
        request_->pimpl_->request_args_.end(),
        [](const auto& arg) { return !arg.second.empty(); }
    ));

    LOG_TRACE() << "method=" << request_->GetMethodStr();
    LOG_TRACE() << "request_args:" << request_->pimpl_->request_args_;
    LOG_TRACE() << "headers:" << request_->pimpl_->headers_;
    LOG_TRACE() << "cookies:" << request_->pimpl_->cookies_;

    return request_;
}

void HttpRequestBuilder::ParseCookies() {
    const std::string& cookie = request_->GetHeader(USERVER_NAMESPACE::http::headers::kCookie);
    const char* data = cookie.data();
    const size_t size = cookie.size();
    const char* end = data + size;
    const char* key_begin = data;
    const char* key_end = data;
    bool parse_key = true;
    for (const char* ptr = data; ptr <= end; ++ptr) {
        if (ptr == end || *ptr == ';') {
            const char* value_begin = nullptr;
            const char* value_end = nullptr;
            if (parse_key) {
                key_end = ptr;
                value_begin = value_end = ptr;
            } else {
                value_begin = key_end + 1;
                value_end = ptr;
                Strip(value_begin, value_end);
                if (value_begin + 2 <= value_end && *value_begin == '"' && value_end[-1] == '"') {
                    ++value_begin;
                    --value_end;
                }
            }
            Strip(key_begin, key_end);
            if (key_begin < key_end) {
                request_->pimpl_->cookies_.emplace(
                    std::piecewise_construct, std::tie(key_begin, key_end), std::tie(value_begin, value_end)
                );
            }
            parse_key = true;
            key_begin = ptr + 1;
            continue;
        }
        if (*ptr == '=' && parse_key) {
            parse_key = false;
            key_end = ptr;
            continue;
        }
    }
}

}  // namespace server::http

USERVER_NAMESPACE_END
