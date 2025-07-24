#include <server/middlewares/handler_adapter.hpp>

#include <algorithm>

#include <boost/container/small_vector.hpp>

#include <server/handlers/http_server_settings.hpp>
#include <server/middlewares/misc.hpp>
#include <server/request/internal_request_context.hpp>

#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/serialize/write_to_stream.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/log.hpp>
#include <userver/utils/not_null.hpp>
#include <userver/utils/scope_guard.hpp>
#include <userver/utils/string_literal.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::middlewares {

namespace {

constexpr utils::StringLiteral kTracingTypeRequest = "request";
constexpr utils::StringLiteral kTracingBody = "body";
constexpr utils::StringLiteral kTracingUri = "uri";

constexpr utils::StringLiteral kUserAgentTag = "useragent";
constexpr utils::StringLiteral kAcceptLanguageTag = "acceptlang";

std::string GetHeadersLogString(
    const http::HttpRequest& request,
    const handlers::HeadersWhitelist& headers_whitelist,
    size_t response_data_size_log_limit
) {
    // Sort to prevent flaky headers reordering, appearing and disappearing for different requests.
    using HeaderRef = utils::NotNull<const http::HttpRequest::HeadersMap::const_iterator::value_type*>;
    boost::container::small_vector<HeaderRef, 32> sorted_headers;
    sorted_headers.reserve(request.GetHeaders().size());
    for (const auto& header : request.GetHeaders()) {
        sorted_headers.emplace_back(header);
    }
    std::sort(sorted_headers.begin(), sorted_headers.end(), [](HeaderRef lhs, HeaderRef rhs) {
        return lhs->first < rhs->first;
    });

    formats::json::StringBuilder sb{};
    {
        const formats::json::StringBuilder::ObjectGuard guard{sb};
        bool some_headers_did_not_fit = false;

        auto write_header = [&](std::string_view header_name, std::string_view header_value) {
            if (sb.GetStringView().size() + header_name.size() + header_value.size() <= response_data_size_log_limit) {
                sb.Key(header_name);
                sb.WriteString(header_value);
            } else {
                some_headers_did_not_fit = true;
            }
        };

        // First, output the visible headers
        for (const auto header_ref : sorted_headers) {
            const auto& [header_name, header_value] = *header_ref;
            if (headers_whitelist.find(header_name) != headers_whitelist.end()) {
                write_header(header_name, header_value);
            }
        }

        for (const auto header_ref : sorted_headers) {
            const auto& [header_name, header_value] = *header_ref;
            if (headers_whitelist.find(header_name) == headers_whitelist.end()) {
                write_header(header_name, "***");
            }
        }

        if (some_headers_did_not_fit) {
            sb.Key("HEADERS-DID-NOT-FIT-IN-SIZE-LIMIT");
            sb.WriteBool(true);
        }
    }
    return sb.GetString();
}

// Separate function to avoid heavy computations when the result is not going
// to be logged
logging::LogExtra LogRequestExtra(
    bool need_log_request_headers,
    const http::HttpRequest& http_request,
    std::string_view meta_type,
    const std::string& body_to_log,
    std::uint64_t body_length,
    const handlers::HeadersWhitelist& headers_whitelist,
    size_t request_headers_size_log_limit
) {
    logging::LogExtra log_extra;

    if (need_log_request_headers) {
        log_extra.Extend(
            "request_headers", GetHeadersLogString(http_request, headers_whitelist, request_headers_size_log_limit)
        );
    }
    log_extra.Extend(tracing::kHttpMetaType, std::string{meta_type});
    log_extra.Extend(tracing::kType, std::string{kTracingTypeRequest});
    log_extra.Extend("request_body_length", body_length);
    log_extra.Extend(std::string{kTracingBody}, body_to_log);
    log_extra.Extend(std::string{kTracingUri}, http_request.GetUrl());
    log_extra.Extend(tracing::kHttpMethod, std::string{http_request.GetMethodStr()});

    const auto& request_application = http_request.GetHeader(USERVER_NAMESPACE::http::headers::kXRequestApplication);
    if (!request_application.empty()) {
        log_extra.Extend("request_application", request_application);
    }

    const auto& user_agent = http_request.GetHeader(USERVER_NAMESPACE::http::headers::kUserAgent);
    if (!user_agent.empty()) {
        log_extra.Extend(std::string{kUserAgentTag}, user_agent);
    }
    const auto& accept_language = http_request.GetHeader(USERVER_NAMESPACE::http::headers::kAcceptLanguage);
    if (!accept_language.empty()) {
        log_extra.Extend(std::string{kAcceptLanguageTag}, accept_language);
    }

    return log_extra;
}

}  // namespace

HandlerAdapter::HandlerAdapter(const handlers::HttpHandlerBase& handler) : handler_{handler} {}

void HandlerAdapter::HandleRequest(http::HttpRequest& request, request::RequestContext& context) const {
    {
        // Logically speaking, the handler should be responsible for parsing request
        // data and not us, but we have to log request data AFTER it has been
        // parsed. Request parsing isn't optional, so we can't move it into its own
        // optional middleware, and this is the only place to hook the pipeline
        // reliably.
        const utils::ScopeGuard log_request_scope{[this, &request, &context] { LogRequest(request, context); }};

        ParseRequestData(request, context);
    }

    handler_.HandleHttpRequest(request, context);
}

void HandlerAdapter::ParseRequestData(const http::HttpRequest& request, request::RequestContext& context) const {
    const auto scope_time = tracing::ScopeTime::CreateOptionalScopeTime("http_parse_request_data");
    handler_.ParseRequestData(request, context);
}

void HandlerAdapter::LogRequest(const http::HttpRequest& request, request::RequestContext& context) const {
    const auto& config_snapshot = context.GetInternalContext().GetConfigSnapshot();

    if (config_snapshot[::dynamic_config::USERVER_LOG_REQUEST]) {
        const bool need_log_request_headers = config_snapshot[::dynamic_config::USERVER_LOG_REQUEST_HEADERS];

        const auto& header_whitelist = config_snapshot[::dynamic_config::USERVER_LOG_REQUEST_HEADERS_WHITELIST];

        const std::string_view meta_type =
            misc::CutTrailingSlash(request.GetRequestPath(), handler_.GetConfig().url_trailing_slash);

        LOG_INFO("start handling {} {}", request.GetMethodStr(), meta_type) << LogRequestExtra(
            need_log_request_headers,
            request,
            meta_type,
            handler_.GetRequestBodyForLoggingChecked(request, context, request.RequestBody()),
            request.RequestBody().length(),
            header_whitelist,
            handler_.GetConfig().request_headers_size_log_limit
        );
    }
}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
