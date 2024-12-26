#include "http_request_constructor.hpp"

#include <http_parser.h>

#include <algorithm>

#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/encoding/hex.hpp>
#include <userver/utils/exception.hpp>

#include "multipart_form_data_parser.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

void StripDuplicateStartingSlashes(std::string& s) {
    if (s.empty() || s[0] != '/') return;

    size_t non_slash_pos = s.find_first_not_of('/');
    if (non_slash_pos == std::string::npos) {
        // all symbols are slashes
        non_slash_pos = s.size();
    }

    if (non_slash_pos == 1) {
        // fast path, no strip
        return;
    }

    s = s.substr(non_slash_pos - 1);
}

}  // namespace

struct HttpRequestConstructor::HttpParserUrl {
    http_parser_url parsed_url;
};

HttpRequestConstructor::HttpRequestConstructor(
    Config config,
    const HandlerInfoIndex& handler_info_index,
    request::ResponseDataAccounter& data_accounter,
    engine::io::Sockaddr remote_address
)
    : config_(config), handler_info_index_(handler_info_index), builder_(data_accounter) {
    builder_.SetRemoteAddress(std::move(remote_address));
}

HttpRequestConstructor::~HttpRequestConstructor() = default;

void HttpRequestConstructor::SetMethod(HttpMethod method) { builder_.SetMethod(method); }

void HttpRequestConstructor::SetHttpMajor(unsigned short http_major) { builder_.SetHttpMajor(http_major); }

void HttpRequestConstructor::SetHttpMinor(unsigned short http_minor) { builder_.SetHttpMinor(http_minor); }

void HttpRequestConstructor::AppendUrl(const char* data, size_t size) {
    // using common limits in checks
    AccountUrlSize(size);
    AccountRequestSize(size);

    url_.append(data, size);
}

void HttpRequestConstructor::ParseUrl() {
    LOG_TRACE() << "parse path from '" << url_ << '\'';
    if (http_parser_parse_url(
            url_.data(),
            url_.size(),
            builder_.GetRef().GetMethod() == HttpMethod::kConnect,
            &parsed_url_pimpl_->parsed_url
        )) {
        SetStatus(Status::kParseUrlError);
        throw std::runtime_error("error in http_parser_parse_url() for url '" + url_ + '\'');
    }

    if (parsed_url_pimpl_->parsed_url.field_set & (1 << http_parser_url_fields::UF_PATH)) {
        const auto& str_info = parsed_url_pimpl_->parsed_url.field_data[http_parser_url_fields::UF_PATH];

        auto request_path = url_.substr(str_info.off, str_info.len);
        StripDuplicateStartingSlashes(request_path);
        LOG_TRACE() << "path='" << request_path << '\'';
        builder_.SetRequestPath(std::move(request_path));
    } else {
        SetStatus(Status::kParseUrlError);
        throw std::runtime_error("no path in url '" + url_ + '\'');
    }

    auto& request = builder_.GetRef();

    auto match_result = handler_info_index_.MatchRequest(request.GetMethod(), request.GetRequestPath());
    const auto* handler_info = match_result.handler_info;

    builder_.SetPathArgs(std::move(match_result.args_from_path));

    if (!handler_info && request.GetMethod() == HttpMethod::kOptions &&
        match_result.status == MatchRequestResult::Status::kMethodNotAllowed) {
        handler_info = handler_info_index_.GetFallbackHandler(handlers::FallbackHandler::kImplicitOptions);
        if (handler_info) match_result.status = MatchRequestResult::Status::kOk;
    }

    if (handler_info) {
        UASSERT(match_result.status == MatchRequestResult::Status::kOk);
        const auto& handler_config = handler_info->handler.GetConfig();
        config_.max_request_size = handler_config.request_config.max_request_size;
        config_.max_headers_size = handler_config.request_config.max_headers_size;
        config_.parse_args_from_body = handler_config.request_config.parse_args_from_body;
        if (handler_config.decompress_request) config_.decompress_request = true;

        builder_.SetTaskProcessor(handler_info->task_processor);
        builder_.SetHttpHandler(handler_info->handler);
        builder_.SetHttpHandlerStatistics(handler_info->handler.GetRequestStatistics());
    } else {
        if (match_result.status == MatchRequestResult::Status::kMethodNotAllowed) {
            SetStatus(Status::kMethodNotAllowed);
        } else {
            SetStatus(Status::kHandlerNotFound);
            LOG_WARNING() << "URL \"" << url_ << "\" not found";
        }
    }

    // recheck sizes using per-handler limits
    AccountUrlSize(0);
    AccountRequestSize(0);

    builder_.SetUrl(std::move(url_));
    url_parsed_ = true;
}

void HttpRequestConstructor::AppendHeaderField(const char* data, size_t size) {
    if (header_value_flag_) {
        AddHeader();
        header_value_flag_ = false;
    }
    header_field_flag_ = true;

    AccountHeadersSize(size);
    AccountRequestSize(size);

    header_field_.append(data, size);
}

void HttpRequestConstructor::AppendHeaderValue(const char* data, size_t size) {
    UASSERT(header_field_flag_);
    header_value_flag_ = true;

    AccountHeadersSize(size);
    AccountRequestSize(size);

    header_value_.append(data, size);
}

void HttpRequestConstructor::AppendBody(const char* data, size_t size) {
    AccountRequestSize(size);
    body_ += std::string_view{data, size};
}

void HttpRequestConstructor::SetIsFinal(bool is_final) { builder_.SetIsFinal(is_final); }

void HttpRequestConstructor::SetResponseStreamId(std::int32_t stream_id) { builder_.SetResponseStreamId(stream_id); }

void HttpRequestConstructor::SetStreamProducer(impl::Http2StreamEventProducer&& producer) {
    builder_.SetStreamProducer(std::move(producer));
}

std::shared_ptr<http::HttpRequest> HttpRequestConstructor::Finalize() {
    FinalizeImpl();

    CheckStatus();

    return builder_.Build();
}

void HttpRequestConstructor::FinalizeImpl() {
    builder_.SetBody(std::move(body_));

    if (status_ != Status::kOk && (!config_.testing_mode || status_ != Status::kHandlerNotFound)) {
        return;
    }

    if (!url_parsed_) {
        SetStatus(Status::kBadRequest);
        return;
    }

    const auto& request = builder_.GetRef();

    try {
        ParseArgs(*parsed_url_pimpl_);
        if (config_.parse_args_from_body) {
            if (!config_.decompress_request || !request.IsBodyCompressed())
                ParseArgs(request.RequestBody().data(), request.RequestBody().size());
        }
    } catch (const std::exception& ex) {
        LOG_WARNING() << "can't parse args: " << ex;
        SetStatus(Status::kParseArgsError);
        return;
    }

    // TODO: split logic
    const auto& content_type = request.GetHeader(USERVER_NAMESPACE::http::headers::kContentType);
    if (IsMultipartFormDataContentType(content_type)) {
        utils::impl::TransparentMap<std::string, std::vector<FormDataArg>, utils::StrCaseHash> form_data_args;
        if (!ParseMultipartFormData(content_type, request.RequestBody(), form_data_args)) {
            SetStatus(Status::kParseMultipartFormDataError);
        } else {
            builder_.SetFormDataArgs(std::move(form_data_args));
        }
    }
}

void HttpRequestConstructor::ParseArgs(const HttpParserUrl& url) {
    if (url.parsed_url.field_set & (1 << http_parser_url_fields::UF_QUERY)) {
        const auto& str_info = url.parsed_url.field_data[http_parser_url_fields::UF_QUERY];
        ParseArgs(builder_.GetRef().GetUrl().data() + str_info.off, str_info.len);
    }
}

void HttpRequestConstructor::ParseArgs(const char* data, size_t size) {
    USERVER_NAMESPACE::http::parser::ParseAndConsumeArgs(
        std::string_view(data, size),
        [this](std::string&& key, std::string&& value) { builder_.AddRequestArg(std::move(key), std::move(value)); }
    );
}

void HttpRequestConstructor::AddHeader() {
    UASSERT(header_field_flag_);

    try {
        builder_.AddHeader(std::move(header_field_), std::move(header_value_));
    } catch (const USERVER_NAMESPACE::http::headers::HeaderMap::TooManyHeadersException&) {
        SetStatus(Status::kHeadersTooLarge);
        utils::LogErrorAndThrow(fmt::format(
            "HeaderMap reached its maximum capacity, already contains {} headers", builder_.GetRef().GetHeaders().size()
        ));
    }
    header_field_.clear();
    header_value_.clear();
}

void HttpRequestConstructor::SetStatus(HttpRequestConstructor::Status status) { status_ = status; }

void HttpRequestConstructor::AccountRequestSize(size_t size) {
    request_size_ += size;
    if (request_size_ > config_.max_request_size) {
        SetStatus(Status::kRequestTooLarge);
        utils::LogErrorAndThrow(
            "request is too large, " + std::to_string(request_size_) + ">" + std::to_string(config_.max_request_size) +
            " (enforced by 'max_request_size' handler limit in config.yaml)" + ", url: " +
            (url_parsed_ ? builder_.GetRef().GetUrl() : "not parsed yet") + ", added size " + std::to_string(size)
        );
    }
}

void HttpRequestConstructor::AccountUrlSize(size_t size) {
    url_size_ += size;
    if (url_size_ > config_.max_url_size) {
        SetStatus(Status::kUriTooLong);
        utils::LogErrorAndThrow(
            "url is too long " + std::to_string(url_size_) + ">" + std::to_string(config_.max_url_size) +
            " (enforced by 'max_url_size' handler limit in "
            "config.yaml)"
        );
    }
}

void HttpRequestConstructor::AccountHeadersSize(size_t size) {
    headers_size_ += size;
    if (headers_size_ > config_.max_headers_size) {
        SetStatus(Status::kHeadersTooLarge);
        utils::LogErrorAndThrow(
            "headers too large " + std::to_string(headers_size_) + ">" + std::to_string(config_.max_headers_size) +
            " (enforced by 'max_headers_size' handler limit in "
            "config.yaml)" +
            ", url: " + builder_.GetRef().GetUrl() + ", added size " + std::to_string(size)
        );
    }
}

void HttpRequestConstructor::CheckStatus() {
    switch (status_) {
        case Status::kOk:
            builder_.SetResponseStatus(HttpStatus::kOk);
            break;
        case Status::kBadRequest:
            builder_.SetResponseStatus(HttpStatus::kBadRequest);
            builder_.GetHttpResponse().SetData("bad request");
            builder_.GetHttpResponse().SetReady();
            break;
        case Status::kUriTooLong:
            builder_.SetResponseStatus(HttpStatus::kUriTooLong);
            builder_.GetHttpResponse().SetReady();
            break;
        case Status::kParseUrlError:
            builder_.SetResponseStatus(HttpStatus::kBadRequest);
            builder_.GetHttpResponse().SetData("invalid url");
            builder_.GetHttpResponse().SetReady();
            break;
        case Status::kHandlerNotFound:
            builder_.SetResponseStatus(HttpStatus::kNotFound);
            builder_.GetHttpResponse().SetReady();
            break;
        case Status::kMethodNotAllowed:
            builder_.SetResponseStatus(HttpStatus::kMethodNotAllowed);
            builder_.GetHttpResponse().SetReady();
            break;
        case Status::kHeadersTooLarge:
            builder_.SetResponseStatus(HttpStatus::kRequestHeaderFieldsTooLarge);
            builder_.GetHttpResponse().SetReady();
            break;
        case Status::kRequestTooLarge:
            builder_.SetResponseStatus(HttpStatus::kPayloadTooLarge);
            builder_.GetHttpResponse().SetData("too large request");
            builder_.GetHttpResponse().SetReady();
            break;
        case Status::kParseArgsError:
            builder_.SetResponseStatus(HttpStatus::kBadRequest);
            builder_.GetHttpResponse().SetData("invalid args");
            builder_.GetHttpResponse().SetReady();
            break;
        case Status::kParseCookiesError:
            builder_.SetResponseStatus(HttpStatus::kBadRequest);
            builder_.GetHttpResponse().SetData("invalid cookies");
            builder_.GetHttpResponse().SetReady();
            break;
        case Status::kParseMultipartFormDataError:
            builder_.SetResponseStatus(HttpStatus::kBadRequest);
            builder_.GetHttpResponse().SetData("invalid body of multipart/form-data request");
            builder_.GetHttpResponse().SetReady();
            break;
    }
}

}  // namespace server::http

USERVER_NAMESPACE_END
