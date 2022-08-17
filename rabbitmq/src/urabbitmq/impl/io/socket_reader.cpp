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
      throw std::runtime_error{"Connection is closed by remote"};
    }

    data_.resize(size_ + read);
    std::memcpy(data_.data() + size_, &tmp_buffer_[0], read);
    size_ += read;

    const auto parsed = [this, conn] {
      auto lock = AmqpConnectionLocker{*conn}.Lock({});
      return conn->GetNative().parse(data_.data(), size_);
    }();
    if (parsed != 0) {
      std::memmove(data_.data(), data_.data() + parsed, size_ - parsed);
      size_ -= parsed;
      parent.AccountRead(parsed);
    }

    return true;
  } catch (const std::exception& ex) {
    if (engine::current_task::IsCancelRequested()) {
      // It's fine, we are destroying the connection
      return false;
    }

    LOG_ERROR() << "Failed to read/process data from socket: " << ex.what();
    parent.Invalidate();
    {
      auto lock = AmqpConnectionLocker{*conn}.Lock({});
      conn->GetNative().fail("Underlying connection broke.");
    }
    return false;
  }
}

}  // namespace urabbitmq::impl::io

USERVER_NAMESPACE_END
