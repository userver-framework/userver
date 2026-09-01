#pragma once

/// @file userver/server/request/response_base.hpp
/// @brief @copybrief server::request::ResponseBase

#include <atomic>
#include <chrono>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include <userver/concurrent/queue.hpp>
#include <userver/concurrent/striped_counter.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

/// @cond
// TODO: server internals. remove from a public interface
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
/// @endcond

namespace engine::io {
class RwBase;
}  // namespace engine::io

namespace server::request {

namespace impl {

class ChunkStorage final {
public:
    ChunkStorage() = default;
    explicit ChunkStorage(std::string data);
    explicit ChunkStorage(std::shared_ptr<const std::string> data);

    ChunkStorage(const ChunkStorage&) = delete;
    ChunkStorage(ChunkStorage&&) noexcept = default;
    ChunkStorage& operator=(const ChunkStorage&) = delete;
    ChunkStorage& operator=(ChunkStorage&&) noexcept = default;

    bool Empty() const noexcept;
    std::size_t Size() const noexcept;
    std::string_view View() const noexcept;
    const std::string& AsString() const;

private:
    std::variant<std::string, std::shared_ptr<const std::string>> storage_{};
};

}  // namespace impl

class ResponseDataAccounter final {
public:
    void StartRequest(std::chrono::steady_clock::time_point create_time);

    void StopRequest(std::size_t size, std::chrono::steady_clock::time_point create_time);

    void ReaccountRequest(
        std::size_t old_size,
        std::chrono::steady_clock::time_point old_create_time,
        std::size_t new_size,
        std::chrono::steady_clock::time_point new_create_time
    );

    std::size_t GetPendingResponsesSizeInBytes() const { return pending_responses_size_in_bytes_; }

    std::size_t GetPendingResponsesCount() const { return pending_responses_count_.NonNegativeRead(); }

    std::size_t GetMaxPendingResponsesSizeInBytes() const { return max_pending_responses_size_in_bytes_; }

    void SetMaxPendingResponsesSizeInBytes(size_t size) { max_pending_responses_size_in_bytes_ = size; }

    std::chrono::milliseconds GetAvgRequestTime() const;

private:
    std::atomic<std::size_t> pending_responses_size_in_bytes_{0};
    std::atomic<std::size_t> max_pending_responses_size_in_bytes_{std::numeric_limits<std::size_t>::max()};
    concurrent::StripedCounter pending_responses_count_{};
    concurrent::StripedCounter time_sum_{};
};

// TODO: merge with HttpResponse

/// @brief Base class for all the server responses.
class ResponseBase {
public:
    explicit ResponseBase(ResponseDataAccounter& data_accounter);
    ResponseBase(const ResponseBase&) = delete;
    ResponseBase(ResponseBase&&) = delete;
    virtual ~ResponseBase() noexcept;

    void SetData(std::string data);
    /// @brief Sets response body without copying, keeping @a data alive.
    /// Useful for serving cached static content.
    void SetSharedData(std::shared_ptr<const std::string> data);
    const std::string& GetData() const;
    /// @cond
    // For internal use only.
    impl::ChunkStorage ExtractData();
    /// @endcond

    virtual bool IsBodyStreamed() const = 0;
    virtual bool WaitForHeadersEnd() = 0;
    virtual void SetHeadersEnd() = 0;

    /// @cond
    // TODO: server internals. remove from a public interface
    void SetReady();
    void SetReady(std::chrono::steady_clock::time_point now);
    bool IsLimitReached() const;

    bool IsReady() const noexcept { return ready_time_ != kUnset; }
    bool IsSent() const noexcept { return is_sent_; }
    std::size_t GetBytesSent() const noexcept { return bytes_sent_; }
    std::chrono::steady_clock::time_point GetReadyTime() const noexcept { return ready_time_; }
    virtual void SendResponse(engine::io::RwBase& socket) = 0;

    virtual void SetStatusServiceUnavailable() = 0;
    virtual void SetStatusOk() = 0;
    virtual void SetStatusNotFound() = 0;

    // HTTP/2.0 only
    void SetStreamId(std::int32_t stream_id);
    std::optional<std::int32_t> GetStreamId() const { return stream_id_; }
    void SetStreamProdicer(http::impl::Http2StreamEventProducer&& producer);
    http::impl::Http2StreamEventProducer GetStreamProducer();
    /// @endcond

protected:
    ResponseBase(ResponseDataAccounter& data_account, std::chrono::steady_clock::time_point now);

    void SetSendFailed();
    void SetSent(std::size_t bytes_sent);

private:
    void StoreData(impl::ChunkStorage data);

    static constexpr auto kUnset = std::chrono::steady_clock::time_point::min();

    ResponseDataAccounter& accounter_;
    impl::ChunkStorage data_;
    std::chrono::steady_clock::time_point create_time_;
    std::chrono::steady_clock::time_point ready_time_{kUnset};
    std::size_t accounted_size_ = 0;
    std::size_t bytes_sent_ = 0;
    bool is_sent_ = false;
    std::optional<std::int32_t> stream_id_;
    std::optional<http::impl::Http2StreamEventProducer> producer_{};
};

}  // namespace server::request

USERVER_NAMESPACE_END
