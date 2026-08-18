#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

#include <userver/concurrent/queue.hpp>
#include <userver/engine/awaitable.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/future_status.hpp>
#include <userver/engine/io/common.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/server/request/response_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

/// @brief The incoming bytes of an upgraded HTTP/2.0 stream.
///
/// Filled by the connection task from the DATA frames of the stream and drained by the
/// task that runs the tunnelled protocol. Shared between the two because the peer may
/// close the stream (and with it the server::http::Stream) while the tunnelled protocol
/// is still unwinding.
///
/// Deliberately lock-free: a tunnelled protocol is free to poll its input in a busy loop
/// (see @ref websocket::WebSocketConnection::TryRecv), and it must not be able to starve
/// the connection task out of filling the pipe.
class Http2StreamReadPipe final {
public:
    Http2StreamReadPipe();

    /// @name Producer side, called from the connection task only.
    /// @{
    void Push(std::string_view data);

    /// @brief Makes the consumer observe an end of stream. Idempotent.
    void Close();
    /// @}

    /// @name Consumer side, called from the task of the tunnelled protocol only.
    /// @{
    /// @returns whether Close() was called and everything buffered has been read.
    bool IsExhausted();

    /// @returns the number of bytes copied, `0` if nothing is buffered.
    std::size_t ReadBuffered(void* buf, std::size_t len);

    /// @brief Waits until there is something to read or the stream ends.
    [[nodiscard]] engine::FutureStatus WaitForData(engine::Deadline deadline);

    engine::AwaitableToken GetAwaitableToken();
    /// @}

private:
    using Queue = concurrent::SpscQueue<std::string>;

    /// @returns whether `chunk_` holds unread bytes, taking the next chunk if needed.
    bool FetchChunk();

    std::shared_ptr<Queue> queue_;
    Queue::Producer producer_;
    Queue::Consumer consumer_;
    // Consumer-only state.
    std::string chunk_;
    std::size_t pos_in_chunk_{0};

    std::atomic<bool> is_closed_{false};
    // Not auto-resetting: the flag mirrors "readable", and only the consumer clears it,
    // which is also what makes it usable as an awaitable.
    engine::SingleConsumerEvent event_{engine::SingleConsumerEvent::NoAutoReset{}};
};

/// @brief Presents a single upgraded HTTP/2.0 stream as a stream-like object, so that
/// protocols tunnelled over it (websockets of RFC 8441) run unmodified.
///
/// Reads come from Http2StreamReadPipe. Writes are pushed as streaming events and are
/// turned into DATA frames by the connection task, because `nghttp2_session` may only
/// ever be touched there.
class Http2StreamRw final : public engine::io::RwBase {
public:
    Http2StreamRw(
        std::int32_t stream_id,
        std::shared_ptr<Http2StreamReadPipe> pipe,
        impl::Http2StreamEventProducer producer
    );

    ~Http2StreamRw() override;

    bool IsValid() const override;

    std::optional<std::size_t> ReadNoblock(void* buf, std::size_t len) override;
    std::size_t ReadSome(void* buf, std::size_t len, engine::Deadline deadline) override;
    std::size_t ReadAll(void* buf, std::size_t len, engine::Deadline deadline) override;
    [[nodiscard]] bool WaitReadable(engine::Deadline deadline) override;

    using engine::io::RwBase::WriteAll;
    std::size_t WriteAll(const void* buf, std::size_t len, engine::Deadline deadline) override;
    std::size_t WriteAll(std::span<const engine::io::IoData> list, engine::Deadline deadline) override;
    [[nodiscard]] bool WaitWriteable(engine::Deadline deadline) override;

private:
    const std::int32_t stream_id_;
    const std::shared_ptr<Http2StreamReadPipe> pipe_;
    impl::Http2StreamEventProducer producer_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
