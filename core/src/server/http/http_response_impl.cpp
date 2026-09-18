#include <server/http/http_response_impl.hpp>

#include <charconv>
#include <cstring>
#include <limits>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/engine/deadline.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/small_string.hpp>

#include <server/http/http_cached_date.hpp>
#include <server/http/http_response_storage.hpp>
#include <server/request/response_data_accounter.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace impl {

namespace {

constexpr std::string_view kCrlf = "\r\n";
constexpr std::string_view kKeyValueHeaderSeparator = ": ";

constexpr std::string_view kClose = "close";
constexpr std::string_view kKeepAlive = "keep-alive";

struct DecimalString final {
    static constexpr std::size_t kMaxSize = std::numeric_limits<std::size_t>::digits10 + 1;

    char data[kMaxSize]{};
    std::size_t size{0};

    std::string_view ToStringView() const noexcept { return {data, size}; }
};

DecimalString FormatDecimal(const std::size_t value) noexcept {
    DecimalString result;
    const auto to_chars_result = std::to_chars(result.data, result.data + sizeof(result.data), value);
    UASSERT(to_chars_result.ec == std::errc{});
    result.size = static_cast<std::size_t>(to_chars_result.ptr - result.data);
    return result;
}

bool IsBodyForbiddenForStatus(HttpStatus status) {
    return status == HttpStatus::kNoContent || status == HttpStatus::kNotModified ||
           (static_cast<int>(status) >= 100 && static_cast<int>(status) < 200);
}

void AppendToCharArray(char*& data, const std::string_view what) {
    std::memcpy(data, what.begin(), what.size());
    data += what.size();
}

}  // namespace

void OutputHeader(USERVER_NAMESPACE::http::headers::HeadersString& header, std::string_view key, std::string_view val) {
    const auto old_size = header.size();

    header.resize_and_overwrite(
        old_size + key.size() + kKeyValueHeaderSeparator.size() + val.size() + kCrlf.size(),
        [&](char* data, std::size_t size) {
            data += old_size;
            AppendToCharArray(data, key);
            AppendToCharArray(data, kKeyValueHeaderSeparator);
            AppendToCharArray(data, val);
            AppendToCharArray(data, kCrlf);
            return size;
        }
    );
}

Http2StreamEventProducer::Http2StreamEventProducer(Http2StreamEventQueue& queue, engine::SingleConsumerEvent& event)
    : producer_(queue.GetProducer()),
      event_(event)
{}

void Http2StreamEventProducer::PushEvent(Http2StreamEvent event, engine::Deadline deadline) {
    const auto res = producer_.Push(std::move(event), deadline);
    UASSERT(res);
    event_.Send();
}

void Http2StreamEventProducer::CloseStream(std::int32_t id) {
    PushEvent({.stream_id = id, .body_part = "", .is_end = true});
}

}  // namespace impl

HttpResponseImpl::HttpResponseImpl(const HttpRequest& request, request::ResponseDataAccounter& data_accounter)
    : HttpResponse{request, data_accounter}
{}

HttpResponseImpl::HttpResponseImpl(
    const HttpRequest& request,
    request::ResponseDataAccounter& data_accounter,
    std::chrono::steady_clock::time_point now,
    utils::StrCaseHash hasher
)
    : HttpResponse{request, data_accounter, now, hasher}
{}

HttpResponseImpl::~HttpResponseImpl() noexcept = default;

impl::ChunkStorage HttpResponseImpl::ExtractData() { return std::move(data_); }

void HttpResponseImpl::SetReady() { SetReady(std::chrono::steady_clock::now()); }

void HttpResponseImpl::SetReady(std::chrono::steady_clock::time_point now) {
    UASSERT(now != kUnset);
    ready_time_ = now;
}

bool HttpResponseImpl::IsLimitReached() const {
    return accounter_.GetPendingResponsesSizeInBytes() >= accounter_.GetMaxPendingResponsesSizeInBytes();
}

void HttpResponseImpl::SetSendFailed() {
    SetStatus(HttpStatus::kClientClosedRequest);
    SetSent(0);
}

void HttpResponseImpl::SetSent(std::size_t bytes_sent) {
    UASSERT(!is_sent_);
    accounter_.StopRequest(accounted_size_, create_time_);
    bytes_sent_ = bytes_sent;
    is_sent_ = true;
}

void HttpResponseImpl::SetStreamId(std::int32_t stream_id) {
    UASSERT(!stream_id_.has_value());
    stream_id_.emplace(stream_id);
}

void HttpResponseImpl::SetStreamProdicer(impl::Http2StreamEventProducer&& producer) {
    UASSERT(!producer_.has_value());
    producer_.emplace(std::move(producer));
}

impl::Http2StreamEventProducer HttpResponseImpl::GetStreamProducer() {
    UASSERT(producer_);
    return std::move(producer_.value());
}

void HttpResponseImpl::SendResponse(engine::io::RwBase& socket) {
    utils::SmallString<USERVER_NAMESPACE::http::headers::kTypicalHeadersSize> header;

    header.resize_and_overwrite(USERVER_NAMESPACE::http::headers::kTypicalHeadersSize, [&](char* data, std::size_t) {
        char* old_data_pointer = data;
        impl::AppendToCharArray(data, "HTTP/");
        data = fmt::format_to(
            data,
            FMT_COMPILE("{}.{} {} "),
            int{http_major_},
            int{http_minor_},
            static_cast<int>(status_)
        );
        impl::AppendToCharArray(data, StatusCodeString(status_));
        impl::AppendToCharArray(data, impl::kCrlf);
        return data - old_data_pointer;
    });

    system_headers_.erase(USERVER_NAMESPACE::http::headers::kContentLength);
    user_headers_.erase(USERVER_NAMESPACE::http::headers::kContentLength);
    if (!HasHeader(USERVER_NAMESPACE::http::headers::kDate)) {
        impl::OutputHeader(
            header,
            USERVER_NAMESPACE::http::headers::kDate,
            // impl::GetCachedDate() must not cross thread boundaries
            impl::GetCachedDate()
        );
    }
    if (!HasHeader(USERVER_NAMESPACE::http::headers::kContentType)) {
        impl::OutputHeader(header, USERVER_NAMESPACE::http::headers::kContentType, kDefaultContentType);
    }
    system_headers_.OutputInHttpFormat(header);
    user_headers_.OutputInHttpFormat(header);
    if (!HasHeader(USERVER_NAMESPACE::http::headers::kConnection)) {
        impl::OutputHeader(
            header,
            USERVER_NAMESPACE::http::headers::kConnection,
            (is_final_ ? impl::kClose : impl::kKeepAlive)
        );
    }
    for (const auto& cookie : cookies_) {
        const std::size_t old_size = header.size();

        header.resize_and_overwrite(
            old_size + static_cast<std::string_view>(USERVER_NAMESPACE::http::headers::kSetCookie).size() +
                impl::kKeyValueHeaderSeparator.size(),
            [&](char* data, std::size_t size) {
                data += old_size;
                impl::AppendToCharArray(data, USERVER_NAMESPACE::http::headers::kSetCookie);
                impl::AppendToCharArray(data, impl::kKeyValueHeaderSeparator);
                return size;
            }
        );

        cookie.second.AppendToString(header);

        header.append(impl::kCrlf);
    }

    std::size_t sent_bytes{};

    if (IsBodyStreamed() && GetData().empty()) {
        sent_bytes = SetBodyStreamed(socket, header);
    } else {
        // e.g. a CustomHandlerException
        sent_bytes = SetBodyNotStreamed(socket, header);
    }

    SetSent(sent_bytes);
}

std::size_t HttpResponseImpl::SetBodyNotStreamed(
    engine::io::RwBase& socket,
    USERVER_NAMESPACE::http::headers::HeadersString& header
) {
    const bool is_body_forbidden = impl::IsBodyForbiddenForStatus(status_);
    const auto& data = GetData();

    if (!is_body_forbidden) {
        const auto content_length = impl::FormatDecimal(data.size());
        impl::OutputHeader(header, USERVER_NAMESPACE::http::headers::kContentLength, content_length.ToStringView());
    }
    header.append(impl::kCrlf);

    if (is_body_forbidden && !data.empty()) {
        LOG_LIMITED_WARNING()
            << "Non-empty body provided for response with HTTP code " << static_cast<int>(status_)
            << " which does not allow one, it will be dropped";
    }

    ssize_t sent_bytes = 0;
    if (!is_head_request_ && !is_body_forbidden) {
        sent_bytes = socket.WriteAll({{header.data(), header.size()}, {data.data(), data.size()}}, engine::Deadline{});
    } else {
        sent_bytes = socket.WriteAll(header.data(), header.size(), engine::Deadline{});
    }

    return sent_bytes;
}

std::size_t HttpResponseImpl::SetBodyStreamed(
    engine::io::RwBase& socket,
    USERVER_NAMESPACE::http::headers::HeadersString& header
) {
    const bool is_body_forbidden = impl::IsBodyForbiddenForStatus(status_);

    if (!is_body_forbidden) {
        impl::OutputHeader(header, USERVER_NAMESPACE::http::headers::kTransferEncoding, "chunked");
    }

    // headers end marker
    header.append(impl::kCrlf);

    // send HTTP headers
    size_t sent_bytes = socket.WriteAll(header.data(), header.size(), {});
    header.clear();
    header.shrink_to_fit();  // free memory before time-consuming operation

    if (is_body_forbidden) {
        return sent_bytes;
    }

    // Transmit HTTP response body
    std::string body_part;
    // First chunk must be sent without kCrlf
    // because kCrlf was sent with headers
    bool first_chunk_processed = false;
    while (body_stream_->Pop(body_part)) {
        if (body_part.empty()) {
            LOG_DEBUG() << "Zero size body_part in http_response.cpp";
            continue;
        }

        auto size =
            first_chunk_processed
                ? fmt::format("\r\n{:x}\r\n", body_part.size())
                : fmt::format("{:x}\r\n", body_part.size());
        sent_bytes +=
            socket.WriteAll({{size.data(), size.size()}, {body_part.data(), body_part.size()}}, engine::Deadline{});

        first_chunk_processed = true;
    }

    const std::string_view terminating_chunk{first_chunk_processed ? "\r\n0\r\n\r\n" : "0\r\n\r\n"};
    sent_bytes += socket.WriteAll(terminating_chunk.data(), terminating_chunk.size(), {});

    // TODO: exceptions?
    body_stream_producer_.emplace<std::monostate>();
    body_stream_.reset();

    return sent_bytes;
}

void HttpResponseImpl::SetSystemHeadersEnd() { system_headers_ended_ = true; }

void HttpResponseImpl::SetStreamBody() {
    UASSERT(body_stream_producer_.index() == 0);
    if (stream_id_.has_value()) {
        UINVARIANT(false, "Streaming in HTTP/2.0 is not supported currently.");
        body_stream_producer_.emplace<impl::Http2StreamEventProducer>(std::move(producer_.value()));
    } else {
        UASSERT(!body_stream_);
        const auto body_queue = Queue::Create();
        body_stream_.emplace(body_queue->GetConsumer());
        body_stream_producer_.emplace<Queue::Producer>(body_queue->GetProducer());
    }
    is_stream_body_ = true;
}

HttpResponseImpl::Producer HttpResponseImpl::GetBodyProducer() {
    Producer res{};
    std::visit(
        utils::Overloaded{
            [&res](Queue::Producer& p) mutable { res = std::move(p); },
            [&res](impl::Http2StreamEventProducer& p) mutable {
                res.emplace<impl::Http2StreamEventProducer>(std::move(p));
            },
            [](const std::monostate) mutable { UINVARIANT(false, "GetBodyProducer() is called twice"); },

        },
        body_stream_producer_
    );
    body_stream_producer_.emplace<std::monostate>();
    return res;
}

}  // namespace server::http

USERVER_NAMESPACE_END
