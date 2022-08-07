#include "socket_reader.hpp"

#include <cstring>

#include <userver/logging/log.hpp>

#include <urabbitmq/impl/amqp_connection.hpp>
#include <urabbitmq/impl/amqp_connection_handler.hpp>
#include <urabbitmq/impl/io/isocket.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl::io {

SocketReader::SocketReader(AmqpConnectionHandler& parent, ISocket& socket)
    : parent_{parent}, socket_{socket} {}

SocketReader::~SocketReader() { Stop(); }

void SocketReader::Start(AmqpConnection* connection) {
  conn_ = connection;

  reader_task_ = engine::CriticalAsyncNoSpan([this] {
    while (!engine::current_task::IsCancelRequested()) {
      if (!buffer_.Read(socket_, conn_, parent_)) {
        conn_->RunLocked([this] { conn_->GetNative().fail("socket error"); },
                         {});
        parent_.Invalidate();
        break;
      }
    }
  });
}

void SocketReader::Stop() { reader_task_.SyncCancel(); }

SocketReader::Buffer::Buffer() { data_.resize(kTmpBufferSize); }

bool SocketReader::Buffer::Read(ISocket& socket, AmqpConnection* conn,
                                AmqpConnectionHandler& parent) {
  try {
    const auto read = socket.RecvSome(&tmp_buffer_[0], kTmpBufferSize);
    if (read == 0) {
      return false;
    }

    data_.resize(size_ + read);
    std::memcpy(data_.data() + size_, &tmp_buffer_[0], read);
    size_ += read;

    const auto parsed = conn->RunLocked(
        [this, conn] { return conn->GetNative().parse(data_.data(), size_); },
        {});
    if (parsed != 0) {
      std::memmove(data_.data(), data_.data() + parsed, size_ - parsed);
      size_ -= parsed;
      parent.AccountRead(parsed);
    }

    return true;
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Failed to read/process data from socket: " << ex;
    return false;
  }
}

}  // namespace urabbitmq::impl::io

USERVER_NAMESPACE_END
