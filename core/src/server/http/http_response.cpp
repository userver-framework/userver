#include <userver/server/http/http_response.hpp>

#include <array>
#include <charconv>
#include <limits>

#include <fmt/format.h>

#include <userver/hostinfo/blocking/get_hostname.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/content_type.hpp>
#include <userver/logging/log.hpp>
#include <userver/tracing/set_throttle_reason.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/overloaded.hpp>

#include <userver/server/http/http_request.hpp>

#include <server/request/response_data_accounter.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace impl {

namespace {

// Guarantees that time_point::min() / kUnset can be used as a sentinel (before the epoch).
static_assert(std::chrono::steady_clock::duration::min() < std::chrono::steady_clock::duration::zero());
static_assert(std::chrono::steady_clock::time_point::min() < std::chrono::steady_clock::time_point{});

const std::string kHostname = hostinfo::blocking::GetRealHostName();

void CheckHeaderName(std::string_view name) {
    static constexpr auto init = []() {
        std::array<uint8_t, 256> res{};  // zero initialize
        for (int i = 0; i < 32; i++) {
            res[i] = 1;
        }
        for (int i = 127; i < 256; i++) {
            res[i] = 1;
        }
        for (const unsigned char c : "()<>@,;:\\\"/[]?={} \t") {
            res[c] = 1;
        }
        return res;
    };
    static constexpr auto bad_chars = init();

    bool check_failed = false;

    // this gets autovectorized, and we optimize for happy path here
    for (const char c : name) {
        const auto code = static_cast<uint8_t>(c);
        check_failed |= bad_chars[code];
    }

    if (check_failed) {
        // in a presumably rare scenarios of the check failing we do a second loop
        for (const char c : name) {
            const auto code = static_cast<uint8_t>(c);
            if (bad_chars[code]) {
                throw std::runtime_error(
                    fmt::format("invalid character in header name: '{}' (#{}), full header name: {}", c, code, name)
                );
            }
        }
    }
}

void CheckHeaderValue(std::string_view value) {
    bool check_failed = false;

    // this gets autovectorized, and we optimize for happy path here
    for (const char c : value) {
        auto code = static_cast<uint8_t>(c);
        check_failed |= code < 32 || code == 127;
    }

    if (check_failed) {
        // in a presumably rare scenarios of the check failing we do a second loop
        for (const char c : value) {
            auto code = static_cast<uint8_t>(c);
            if (code < 32 || code == 127) {
                throw std::runtime_error(
                    std::string("invalid character in header value: '") + c + "' (#" + std::to_string(code) + ")"
                );
            }
        }
    }
}

const std::string kEmptyString{};

}  // namespace

bool ChunkStorage::Empty() const noexcept { return Size() == 0; }

std::size_t ChunkStorage::Size() const noexcept {
    return std::visit(
        utils::Overloaded{
            [](const std::string& owned) noexcept { return owned.size(); },
            [](const std::shared_ptr<const std::string>& shared) noexcept { return shared->size(); },
        },
        storage_
    );
}

std::string_view ChunkStorage::View() const noexcept { return AsString(); }

const std::string& ChunkStorage::AsString() const {
    return std::visit(
        utils::Overloaded{
            [](const std::string& owned) -> const std::string& { return owned; },
            [](const std::shared_ptr<const std::string>& shared) -> const std::string& { return *shared; },
        },
        storage_
    );
}

ChunkStorage::ChunkStorage(std::string data)
    : storage_{std::move(data)}
{}

ChunkStorage::ChunkStorage(std::shared_ptr<const std::string> data)
    : storage_{std::move(data)}
{
    UASSERT(std::get<std::shared_ptr<const std::string>>(storage_));
}

}  // namespace impl

HttpResponse::HttpResponse(const HttpRequest& request, request::ResponseDataAccounter& data_accounter)
    : HttpResponse{request, data_accounter, std::chrono::steady_clock::now(), utils::StrCaseHash{}}
{}

HttpResponse::HttpResponse(
    const HttpRequest& request,
    request::ResponseDataAccounter& data_accounter,
    std::chrono::steady_clock::time_point now,
    utils::StrCaseHash hasher
)
    : accounter_{data_accounter},
      create_time_{now},
      is_final_(request.IsFinal()),
      is_head_request_(request.GetMethod() == HttpMethod::kHead),
      http_major_(request.GetHttpMajor()),
      http_minor_(request.GetHttpMinor()),
      cookies_{0, hasher}
{
    UASSERT(accounted_size_ == 0);
    UASSERT(data_.Empty());
    accounter_.StartRequest(create_time_);
}

HttpResponse::~HttpResponse() noexcept {
    if (!is_sent_) {
        accounter_.StopRequest(accounted_size_, create_time_);
    }
}

void HttpResponse::SetData(std::string data) { StoreData(impl::ChunkStorage{std::move(data)}); }

void HttpResponse::SetSharedData(std::shared_ptr<const std::string> data) {
    UASSERT(data);
    StoreData(impl::ChunkStorage{std::move(data)});
}

void HttpResponse::StoreData(impl::ChunkStorage data) {
    if (is_sent_) {
        UASSERT(is_stream_body_);
        LOG_LIMITED_WARNING()
            << "Attempt to set response body after it was already sent by streaming. Probably an "
               "exception was thrown after streaming started";
        return;
    }
    data_ = std::move(data);
    const auto old_size = accounted_size_;
    const auto old_create_time = create_time_;
    create_time_ = std::chrono::steady_clock::now();
    accounted_size_ = data_.Size();
    accounter_.ReaccountRequest(old_size, old_create_time, accounted_size_, create_time_);
}

const std::string& HttpResponse::GetData() const { return data_.AsString(); }

bool HttpResponse::SetHeader(std::string name, std::string value) {
    if (!IsHeadersMutable()) {
        // Attempt to set headers for Stream'ed response after it is already set
        return false;
    }

    impl::CheckHeaderName(name);
    impl::CheckHeaderValue(value);

    if (system_headers_ended_) {
        if (system_headers_.contains(name)) {
            LOG_DEBUG() << "User response header is not set, because it overrides system header: " << name;
            return false;
        }
        user_headers_.insert_or_assign(std::move(name), std::move(value));
    } else {
        system_headers_.insert_or_assign(std::move(name), std::move(value));
    }

    return true;
}

bool HttpResponse::SetHeader(std::string_view name, std::string value) {
    return SetHeader(std::string{name}, std::move(value));
}

bool HttpResponse::SetHeader(const USERVER_NAMESPACE::http::headers::PredefinedHeader& header, std::string value) {
    if (!IsHeadersMutable()) {
        // Attempt to set headers for Stream'ed response after it is already set
        return false;
    }

    impl::CheckHeaderValue(value);

    if (system_headers_ended_) {
        if (system_headers_.contains(header)) {
            LOG_DEBUG()
                << "User response header is not set, because it overrides system header: " << std::string_view(header);
            return false;
        }
        user_headers_.insert_or_assign(header, std::move(value));
    } else {
        system_headers_.insert_or_assign(header, std::move(value));
    }

    return true;
}

void HttpResponse::SetContentType(const USERVER_NAMESPACE::http::ContentType& type) {
    SetHeader(USERVER_NAMESPACE::http::headers::kContentType, type.ToString());
}

void HttpResponse::SetContentEncoding(std::string encoding) {
    SetHeader(USERVER_NAMESPACE::http::headers::kContentEncoding, std::move(encoding));
}

bool HttpResponse::SetStatus(HttpStatus status) noexcept {
    if (!IsHeadersMutable()) {
        // Attempt to set headers for Stream'ed response after it is already set
        return false;
    }

    status_ = status;
    return true;
}

bool HttpResponse::ClearUserHeaders() {
    if (!IsHeadersMutable()) {
        // Attempt to set headers for Stream'ed response after it is already set
        return false;
    }

    user_headers_.clear();
    return true;
}

void HttpResponse::SetCookie(Cookie cookie) {
    impl::CheckHeaderValue(cookie.Name());
    impl::CheckHeaderValue(cookie.Value());
    UASSERT(!cookie.Name().empty());
    auto [it, ok] = cookies_.emplace(std::string_view{}, std::move(cookie));
    UASSERT(ok);

    auto node = cookies_.extract(it);
    UASSERT(node);
    node.key() = node.mapped().Name();
    cookies_.insert(std::move(node));
}

void HttpResponse::ClearCookies() { cookies_.clear(); }

HttpResponse::HeadersMapKeys HttpResponse::GetSystemHeaderNames() const {
    return HttpResponse::HeadersMapKeys{system_headers_};
}

HttpResponse::HeadersMapKeys HttpResponse::GetUserHeaderNames() const {
    return HttpResponse::HeadersMapKeys{user_headers_};
}

const std::string& HttpResponse::GetHeader(std::string_view header_name) const {
    auto it = user_headers_.find(header_name);
    if (it != user_headers_.end()) {
        return it->second;
    }
    it = system_headers_.find(header_name);
    if (it != system_headers_.end()) {
        return it->second;
    }
    return impl::kEmptyString;
}

const std::string& HttpResponse::GetHeader(const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name
) const {
    auto it = user_headers_.find(header_name);
    if (it != user_headers_.end()) {
        return it->second;
    }
    it = system_headers_.find(header_name);
    if (it != system_headers_.end()) {
        return it->second;
    }
    return impl::kEmptyString;
}

bool HttpResponse::HasHeader(std::string_view header_name) const {
    return user_headers_.find(header_name) != user_headers_.end() ||
           system_headers_.find(header_name) != system_headers_.end();
}

bool HttpResponse::HasHeader(const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name) const {
    return user_headers_.find(header_name) != user_headers_.end() ||
           system_headers_.find(header_name) != system_headers_.end();
}

HttpResponse::CookiesMapKeys HttpResponse::GetCookieNames() const { return HttpResponse::CookiesMapKeys{cookies_}; }

const Cookie& HttpResponse::GetCookie(std::string_view cookie_name) const { return cookies_.at(cookie_name.data()); }

void HttpResponse::SetHeadersEnd() { headers_end_.Send(); }

bool HttpResponse::WaitForHeadersEnd() { return headers_end_.WaitForEvent(); }

void SetThrottleReason(http::HttpResponse& http_response, std::string log_reason, std::string http_header_reason) {
    http_response.SetHeader(USERVER_NAMESPACE::http::headers::kXYaTaxiRatelimitedBy, impl::kHostname);
    http_response.SetHeader(USERVER_NAMESPACE::http::headers::kXYaTaxiRatelimitReason, std::move(http_header_reason));

    if (auto* span = tracing::Span::CurrentSpanUnchecked()) {
        tracing::SetThrottleReason(*span, std::move(log_reason));
    }
}

}  // namespace server::http

USERVER_NAMESPACE_END
