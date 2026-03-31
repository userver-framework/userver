#include <server/net/http_socket_reader.hpp>

#include <userver/engine/io/tls_wrapper.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::net {

SocketBufferedReader::SocketBufferedReader(const ConnectionConfig& config, std::string peer_name)
    : keepalive_timeout_(config.keepalive_timeout),
      peer_name_(std::move(peer_name))
{
    pending_data_.resize(config.in_buffer_size);
}

bool SocketBufferedReader::TryRead(engine::io::RwBase& peer_socket, int fd) {
    if (IsFull()) {
        // Buffer is full => force that we read it.
        return true;
    }
    const auto deadline = engine::Deadline::FromDuration(keepalive_timeout_);
    // We almost certainly will hit EWOULDBLOCK on the subsequent recv syscall from
    // peer_socket_.RecvSome, which will fall back to event-loop waiting
    // for socket to become readable, and then issue another recv syscall,
    // effectively doing
    // 1. recv (returns -1)
    // 2. notify event-loop about read interest
    // 3. recv (return some data)
    //
    // So instead we just do 2. and 3., shaving off a whole recv syscall
    if (peer_socket.WaitReadable(deadline)) {
        return DoRead(peer_socket, deadline, fd) > 0;
    }
    return false;
}

std::size_t SocketBufferedReader::DoRead(engine::io::RwBase& peer_socket, engine::Deadline deadline, int fd) {
    UASSERT(!IsFull());
    const auto count =
        peer_socket
            .ReadSome(pending_data_.data() + pending_data_size_, pending_data_.size() - pending_data_size_, deadline);
    pending_data_size_ += count;
    if (count > 0) {
        LOG_TRACE() << "Received " << pending_data_size_ << " byte(s) from " << GetPeerName() << " on fd " << fd;
    } else {
        LOG_TRACE() << "Peer " << GetPeerName() << " on fd " << fd << " closed connection or the connection timed out";
    }
    return count;
}

std::string_view SocketBufferedReader::PeekAsStringView() const noexcept {
    return {pending_data_.data(), pending_data_size_};
}

void SocketBufferedReader::ResetView() noexcept { pending_data_size_ = 0; }

bool SocketBufferedReader::IsEmpty() const noexcept { return BufferSize() == 0; }

bool SocketBufferedReader::IsFull() const noexcept { return pending_data_size_ == pending_data_.size(); }

std::size_t SocketBufferedReader::BufferSize() const noexcept { return pending_data_size_; }

const std::string& SocketBufferedReader::GetPeerName() const noexcept { return peer_name_; }

}  // namespace server::net

USERVER_NAMESPACE_END
