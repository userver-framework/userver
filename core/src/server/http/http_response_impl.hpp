#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/concurrent/queue.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/single_consumer_event.hpp>

#include <server/http/http_response_storage.hpp>

#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_response.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http::impl {

struct Http2StreamEvent {
    std::int32_t stream_id{-1};
    std::string body_part{};
    bool is_end{false};
};

// The order is fifo in the context of a single producer. So we are tolerant to
// reordering between producers
using Http2StreamEventQueue = concurrent::NonFifoMpscQueue<Http2StreamEvent>;

class Http2StreamEventProducer final {
public:
    Http2StreamEventProducer(Http2StreamEventQueue& queue, engine::SingleConsumerEvent& event);

    void PushEvent(Http2StreamEvent event, engine::Deadline deadline = {});

    void CloseStream(std::int32_t id);

private:
    Http2StreamEventQueue::Producer producer_;
    engine::SingleConsumerEvent& event_;
};

}  // namespace server::http::impl

namespace server::http {

class HttpResponseImpl final : public HttpResponse {
public:
    HttpResponseImpl(const HttpRequest& request, request::ResponseDataAccounter& data_accounter);
    HttpResponseImpl(
        const HttpRequest& request,
        request::ResponseDataAccounter& data_accounter,
        std::chrono::steady_clock::time_point now,
        utils::StrCaseHash hasher
    );
    ~HttpResponseImpl() noexcept;

    void SetSendFailed();

    impl::ChunkStorage ExtractData();

    [[nodiscard]] impl::HeadersEndEvent FinishedSendingHeadersEvent() noexcept USERVER_IMPL_LIFETIME_BOUND {
        return impl::HeadersEndEvent{headers_end_};
    }

    void SendResponse(engine::io::RwBase& socket);

    void SetReady();
    void SetReady(std::chrono::steady_clock::time_point now);
    bool IsLimitReached() const;

    bool IsReady() const noexcept { return ready_time_ != kUnset; }
    std::size_t GetBytesSent() const noexcept { return bytes_sent_; }
    std::chrono::steady_clock::time_point GetReadyTime() const noexcept { return ready_time_; }

    void SetStreamId(std::int32_t stream_id);
    std::optional<std::int32_t> GetStreamId() const { return stream_id_; }
    void SetStreamProdicer(impl::Http2StreamEventProducer&& producer);
    impl::Http2StreamEventProducer GetStreamProducer();

    void SetSent(std::size_t bytes_sent);

    using Queue = concurrent::StringStreamQueue;
    using Producer = std::variant<std::monostate, Queue::Producer, impl::Http2StreamEventProducer>;

    void SetStreamBody();
    // Can be called only once
    Producer GetBodyProducer();

    /// @brief Set the end of system headers.
    /// All headers written before this call are considered system; after - user.
    /// User headers can't overwrite system headers.
    void SetSystemHeadersEnd();

    void SetHeadRequest(bool is_head_request) noexcept { is_head_request_ = is_head_request; }

private:
    std::size_t SetBodyStreamed(engine::io::RwBase& socket, USERVER_NAMESPACE::http::headers::HeadersString& header);

    std::size_t SetBodyNotStreamed(engine::io::RwBase& socket, USERVER_NAMESPACE::http::headers::HeadersString& header);

    std::optional<std::int32_t> stream_id_;
    std::optional<impl::Http2StreamEventProducer> producer_{};

    std::optional<concurrent::StringStreamQueue::Consumer> body_stream_;
    std::variant<std::monostate, concurrent::StringStreamQueue::Producer, impl::Http2StreamEventProducer>
        body_stream_producer_;
};

inline HttpResponseImpl& GetHttpResponseImpl(HttpResponse& response) noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    return static_cast<HttpResponseImpl&>(response);
}

inline const HttpResponseImpl& GetHttpResponseImpl(const HttpResponse& response) noexcept {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
    return static_cast<const HttpResponseImpl&>(response);
}

inline HttpResponseImpl& GetHttpResponseImpl(HttpRequest& request) noexcept {
    return GetHttpResponseImpl(request.GetHttpResponse());
}

inline const HttpResponseImpl& GetHttpResponseImpl(const HttpRequest& request) noexcept {
    return GetHttpResponseImpl(request.GetHttpResponse());
}

}  // namespace server::http

USERVER_NAMESPACE_END
