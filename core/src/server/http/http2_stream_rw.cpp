#include <server/http/http2_stream_rw.hpp>

#include <algorithm>
#include <cstring>

#include <userver/engine/io/exception.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace {

[[noreturn]] void ThrowOnWaitFailure(engine::FutureStatus status, std::size_t bytes_transferred) {
    UASSERT(status != engine::FutureStatus::kReady);
    if (status == engine::FutureStatus::kCancelled) {
        throw engine::io::IoCancelled{bytes_transferred};
    }
    throw engine::io::IoTimeout{bytes_transferred};
}

}  // namespace

Http2StreamReadPipe::Http2StreamReadPipe()
    : queue_(Queue::Create()),
      producer_(queue_->GetProducer()),
      consumer_(queue_->GetConsumer())
{
    // The peer is already bounded by the flow control window of the stream, and the
    // producer runs in the connection task, where it must never block.
    queue_->SetSoftMaxSize(Queue::kUnbounded);
}

void Http2StreamReadPipe::Push(std::string_view data) {
    if (data.empty()) {
        return;
    }
    if (!producer_.PushNoblock(std::string{data})) {
        // Only happens once the consumer is gone, i.e. the tunnelled protocol is over.
        LOG_LIMITED_DEBUG() << "Dropping the incoming bytes of an abandoned upgraded stream";
        return;
    }
    event_.Send();
}

void Http2StreamReadPipe::Close() {
    is_closed_.store(true, std::memory_order_release);
    event_.Send();
}

bool Http2StreamReadPipe::FetchChunk() {
    if (pos_in_chunk_ < chunk_.size()) {
        return true;
    }
    chunk_.clear();
    pos_in_chunk_ = 0;
    // Push() never enqueues an empty chunk, so a successful pop always means data.
    return consumer_.PopNoblock(chunk_);
}

bool Http2StreamReadPipe::IsExhausted() {
    // Drain first: the last chunks may have been pushed just before the close.
    return !FetchChunk() && is_closed_.load(std::memory_order_acquire);
}

std::size_t Http2StreamReadPipe::ReadBuffered(void* buf, std::size_t len) {
    auto* out = static_cast<char*>(buf);
    std::size_t copied = 0;
    while (copied < len && FetchChunk()) {
        const auto part = std::min(len - copied, chunk_.size() - pos_in_chunk_);
        std::memcpy(out + copied, chunk_.data() + pos_in_chunk_, part);
        copied += part;
        pos_in_chunk_ += part;
    }
    return copied;
}

engine::FutureStatus Http2StreamReadPipe::WaitForData(engine::Deadline deadline) {
    if (FetchChunk() || is_closed_.load(std::memory_order_acquire)) {
        return engine::FutureStatus::kReady;
    }
    // Reset first and re-check afterwards: a producer that fills the pipe after the
    // reset signals again, so no wakeup can be lost.
    event_.Reset();
    if (FetchChunk() || is_closed_.load(std::memory_order_acquire)) {
        return engine::FutureStatus::kReady;
    }
    return event_.WaitUntil(deadline);
}

engine::AwaitableToken Http2StreamReadPipe::GetAwaitableToken() { return event_.GetAwaitableToken(); }

Http2StreamRw::Http2StreamRw(
    std::int32_t stream_id,
    std::shared_ptr<Http2StreamReadPipe> pipe,
    impl::Http2StreamEventProducer producer
)
    : stream_id_(stream_id),
      pipe_(std::move(pipe)),
      producer_(std::move(producer))
{
    UASSERT(pipe_);
    SetReadableAwaitableToken(pipe_->GetAwaitableToken());
    // Writes only enqueue an event for the connection task, so they never block.
    SetWritableAwaitableToken(engine::MakeReadyAwaitableToken());
}

Http2StreamRw::~Http2StreamRw() = default;

bool Http2StreamRw::IsValid() const { return !pipe_->IsExhausted(); }

std::optional<std::size_t> Http2StreamRw::ReadNoblock(void* buf, std::size_t len) {
    if (len == 0) {
        return std::size_t{0};
    }
    if (const auto read = pipe_->ReadBuffered(buf, len); read != 0) {
        return read;
    }
    if (pipe_->IsExhausted()) {
        return std::size_t{0};
    }
    // Unlike a socket, the pipe is filled by another task of this very service, and a
    // caller polling it in a busy loop (websocket::WebSocketConnection::TryRecv) would
    // otherwise never let the connection task run.
    engine::Yield();
    return std::nullopt;
}

std::size_t Http2StreamRw::ReadSome(void* buf, std::size_t len, engine::Deadline deadline) {
    if (len == 0) {
        return 0;
    }
    while (true) {
        if (const auto read = pipe_->ReadBuffered(buf, len); read != 0) {
            return read;
        }
        if (pipe_->IsExhausted()) {
            return 0;
        }
        if (const auto status = pipe_->WaitForData(deadline); status != engine::FutureStatus::kReady) {
            ThrowOnWaitFailure(status, 0);
        }
    }
}

std::size_t Http2StreamRw::ReadAll(void* buf, std::size_t len, engine::Deadline deadline) {
    auto* out = static_cast<char*>(buf);
    std::size_t read = 0;
    while (read < len) {
        std::size_t part = 0;
        try {
            part = ReadSome(out + read, len - read, deadline);
        } catch (const engine::io::IoCancelled&) {
            throw engine::io::IoCancelled{read};
        } catch (const engine::io::IoTimeout&) {
            throw engine::io::IoTimeout{read};
        }
        if (part == 0) {
            break;  // the peer half-closed the stream
        }
        read += part;
    }
    return read;
}

bool Http2StreamRw::WaitReadable(engine::Deadline deadline) {
    return pipe_->WaitForData(deadline) == engine::FutureStatus::kReady;
}

std::size_t Http2StreamRw::WriteAll(const void* buf, std::size_t len, engine::Deadline deadline) {
    if (len == 0) {
        return 0;
    }
    producer_.PushEvent(
        {.stream_id = stream_id_, .body_part = std::string{static_cast<const char*>(buf), len}, .is_end = false},
        deadline
    );
    return len;
}

std::size_t Http2StreamRw::WriteAll(std::span<const engine::io::IoData> list, engine::Deadline deadline) {
    std::size_t total = 0;
    for (const auto& io_data : list) {
        total += io_data.len;
    }
    if (total == 0) {
        return 0;
    }

    // A single event keeps the parts of one websocket frame in one DATA frame.
    std::string body;
    body.reserve(total);
    for (const auto& io_data : list) {
        body.append(static_cast<const char*>(io_data.data), io_data.len);
    }
    producer_.PushEvent({.stream_id = stream_id_, .body_part = std::move(body), .is_end = false}, deadline);
    return total;
}

bool Http2StreamRw::WaitWriteable(engine::Deadline) { return true; }

}  // namespace server::http

USERVER_NAMESPACE_END
