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

constexpr utils::StringLiteral kAcceptLanguageTag = "acceptlang";

std::string GetHeadersLogString(
    const http::HttpRequest& request,
    const handlers::HeadersWhitelist& headers_whitelist,
    size_t response_data_size_log_limit
) {
    constexpr std::string_view kTruncateSuffix = "...(truncated, total {} bytes)";
    constexpr size_t kMaxTruncateSuffixSize = kTruncateSuffix.size() + std::numeric_limits<size_t>::digits10;
    constexpr std::string_view kMask = "***";

    // Sort to prevent flaky headers reordering, appearing and disappearing for different requests.
    using HeaderRef = utils::NotNull<const http::HttpRequest::HeadersMap::const_iterator::value_type*>;
    using IsValueHidden = bool;

    boost::container::small_vector<std::pair<HeaderRef, IsValueHidden>, 32> sorted_headers;
    sorted_headers.reserve(request.GetHeaders().size());
    size_t max_result_size = 0;

    for (const auto& header : request.GetHeaders()) {
        const IsValueHidden header_hide = headers_whitelist.find(header.first) == headers_whitelist.end();
        max_result_size += header.first.size() + (header_hide ? kMask.size() : header.second.size()) + 3;
        sorted_headers.emplace_back(header, header_hide);
    }
    std::sort(sorted_headers.begin(), sorted_headers.end(), [](const auto& lhs, const auto& rhs) {
        return std::tie(lhs.second, *lhs.first) < std::tie(rhs.second, *rhs.first);
    });

    std::string result;
    result.reserve(std::min(max_result_size, response_data_size_log_limit + kMaxTruncateSuffixSize));

    for (const auto& [header_ref, header_hide] : sorted_headers) {
        const auto& [header_name, header_value] = *header_ref;
        const std::string_view header_log_value = header_hide ? kMask : header_value;
        if (result.size() + header_name.size() + header_log_value.size() + 3 > response_data_size_log_limit) {
            result += fmt::format(kTruncateSuffix, max_result_size);
            break;
        }
        for (auto header_part : std::array<std::string_view, 4>{header_name, ": ", header_log_value, "\n"}) {
            result += header_part;
        }
    }

    return result;
}

logging::LogExtra GetHeadersLogExtra(
    bool need_log_request_headers,
    const http::HttpRequest& http_request,
    const handlers::HeadersWhitelist& headers_whitelist,
    size_t request_headers_size_log_limit
) {
    logging::LogExtra headers_log_extra;

    if (need_log_request_headers) {
        headers_log_extra.Extend(
            "request_headers",
            GetHeadersLogString(http_request, headers_whitelist, request_headers_size_log_limit)
        );
    }

    const auto& request_application = http_request.GetHeader(USERVER_NAMESPACE::http::headers::kXRequestApplication);
    if (!request_application.empty()) {
        headers_log_extra.Extend("request_application", request_application);
    }

    const auto& user_agent = http_request.GetHeader(USERVER_NAMESPACE::http::headers::kUserAgent);
    if (!user_agent.empty()) {
        headers_log_extra.Extend(std::string{tracing::kUserAgent}, user_agent);
    }
    const auto& accept_language = http_request.GetHeader(USERVER_NAMESPACE::http::headers::kAcceptLanguage);
    if (!accept_language.empty()) {
        headers_log_extra.Extend(std::string{kAcceptLanguageTag}, accept_language);
    }

    return headers_log_extra;
}

}  // namespace

HandlerAdapter::HandlerAdapter(const handlers::HttpHandlerBase& handler)
    : handler_{handler}
{}

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

        const std::string_view
            meta_type = misc::CutTrailingSlash(request.GetRequestPath(), handler_.GetConfig().url_trailing_slash);

        logging::LogExtra log_extra{
            {tracing::kHttpMetaType, meta_type},
            {tracing::kType, kTracingTypeRequest},
            {"request_body_length", request.RequestBody().length()},
            {kTracingBody, handler_.GetRequestBodyForLoggingChecked(request, context, request.RequestBody())},
            {kTracingUri, handler_.GetUrlForLoggingChecked(request, context)},
            {tracing::kHttpMethod, request.GetMethodStr()},
        };
        log_extra.Extend(GetHeadersLogExtra(
            need_log_request_headers,
            request,
            header_whitelist,
            handler_.GetConfig().request_headers_size_log_limit
        ));

        LOG_INFO("start handling {} {}", request.GetMethodStr(), meta_type) << log_extra;
    }
}

}  // namespace server::middlewares

USERVER_NAMESPACE_END
