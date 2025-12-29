#include <userver/clients/http/middlewares/base.hpp>

#include <clients/http/request_state.hpp>
#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

MiddlewareRequest::MiddlewareRequest(RequestState& state)
    : state_(state)
{}

void MiddlewareRequest::SetHeader(std::string_view name, std::string_view value) {
    state_.easy().add_header(
        name,
        value,
        curl::easy::EmptyHeaderAction::kDoNotSend,
        curl::easy::DuplicateHeaderAction::kReplace
    );
}

void MiddlewareRequest::AddQueryParams(std::string_view params) {
    const auto& url = state_.easy().get_original_url();
    if (url.find('?') != std::string::npos) {
        state_.easy().set_url(utils::StrCat(url, "&", params));
    } else {
        state_.easy().set_url(utils::StrCat(url, "?", params));
    }
}

void MiddlewareRequest::SetTimeout(std::chrono::milliseconds ms) {
    state_.set_timeout(ms.count());
    state_.SetEasyTimeout(ms);
}

void MiddlewareRequest::SetProxy(utils::zstring_view value) { state_.proxy(value); }

bool MiddlewareRequest::IsProxySet() const { return state_.IsProxySet(); }

const std::string& MiddlewareRequest::GetOriginalUrl() const { return state_.easy().get_original_url(); }

MiddlewareBase::MiddlewareBase() = default;

MiddlewareBase::~MiddlewareBase() = default;

void MiddlewareBase::HookPerformRequest(MiddlewareRequest&) {}

void MiddlewareBase::HookCreateSpan(MiddlewareRequest&, tracing::Span&) {}

void MiddlewareBase::HookOnCompleted(MiddlewareRequest&, Response&) {}

void MiddlewareBase::HookOnError(MiddlewareRequest&, std::error_code) {}

bool MiddlewareBase::HookOnRetry(MiddlewareRequest&) { return true; }

}  // namespace clients::http

USERVER_NAMESPACE_END
