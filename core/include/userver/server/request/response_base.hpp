#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <limits>
#include <optional>
#include <string>

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

class ResponseDataAccounter final {
public:
    void StartRequest(size_t size, std::chrono::steady_clock::time_point create_time);

    void StopRequest(size_t size, std::chrono::steady_clock::time_point create_time);

    size_t GetCurrentLevel() const { return current_; }

    size_t GetMaxLevel() const { return max_; }

    void SetMaxLevel(size_t size) { max_ = size; }

    std::chrono::milliseconds GetAvgRequestTime() const;

private:
    std::atomic<size_t> current_{0};
    std::atomic<size_t> max_{std::numeric_limits<size_t>::max()};
    concurrent::StripedCounter count_;
    concurrent::StripedCounter time_sum_;
};

/// @brief Base class for all the server responses.
class ResponseBase {
public:
    explicit ResponseBase(ResponseDataAccounter& data_accounter);
    ResponseBase(const ResponseBase&) = delete;
    ResponseBase(ResponseBase&&) = delete;
    virtual ~ResponseBase() noexcept;

    void SetData(std::string data);
    const std::string& GetData() const { return data_; }
    std::string&& ExtractData() { return std::move(data_); }

    virtual bool IsBodyStreamed() const = 0;
    virtual bool WaitForHeadersEnd() = 0;
    virtual void SetHeadersEnd() = 0;

    /// @cond
    // TODO: server internals. remove from a public interface
    void SetReady();
    void SetReady(std::chrono::steady_clock::time_point now);
    virtual void SetSendFailed(std::chrono::steady_clock::time_point failure_time);
    bool IsLimitReached() const;

    bool IsReady() const { return is_ready_; }
    bool IsSent() const { return is_sent_; }
    size_t BytesSent() const { return bytes_sent_; }
    std::chrono::steady_clock::time_point ReadyTime() const { return ready_time_; }
    std::chrono::steady_clock::time_point SentTime() const { return sent_time_; }
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

    void SetSent(std::size_t bytes_sent, std::chrono::steady_clock::time_point sent_time);

private:
    class Guard final {
    public:
        Guard(ResponseDataAccounter& accounter, std::chrono::steady_clock::time_point create_time, size_t size)
            : accounter_(accounter), create_time_(create_time), size_(size) {
            accounter_.StartRequest(size_, create_time_);
        }

        ~Guard() { accounter_.StopRequest(size_, create_time_); }

    private:
        ResponseDataAccounter& accounter_;
        std::chrono::steady_clock::time_point create_time_;
        size_t size_;
    };

    ResponseDataAccounter& accounter_;
    std::optional<Guard> guard_;
    std::string data_;
    std::chrono::steady_clock::time_point create_time_;
    std::chrono::steady_clock::time_point ready_time_;
    std::chrono::steady_clock::time_point sent_time_;
    size_t bytes_sent_ = 0;
    bool is_ready_ = false;
    bool is_sent_ = false;
    std::optional<std::int32_t> stream_id_;
    std::optional<http::impl::Http2StreamEventProducer> producer_{};
};

}  // namespace server::request

USERVER_NAMESPACE_END
