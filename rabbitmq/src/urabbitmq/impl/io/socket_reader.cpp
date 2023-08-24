#include "socket_reader.hpp"

#include <cstring>

#include <userver/engine/io/common.hpp>
#include <userver/logging/log.hpp>

#include <urabbitmq/impl/amqp_connection.hpp>
#include <urabbitmq/impl/amqp_connection_handler.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl::io {

SocketReader::SocketReader(AmqpConnectionHandler& parent,
                           engine::io::RwBase& socket)
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

bool SocketReader::Buffer::Read(engine::io::RwBase& socket,
                                AmqpConnection* conn,
                                AmqpConnectionHandler& parent) {
  try {
    bool is_readable = true;
    if (last_bytes_read_ != kTmpBufferSize) {
      is_readable = socket.WaitReadable({});
    }

    last_bytes_read_ =
        is_readable ? socket.ReadSome(&tmp_buffer_[0], kTmpBufferSize, {}) : 0;
    if (last_bytes_read_ == 0) {
      throw std::runtime_error{"Connection is closed by remote"};
    }

    data_.resize(size_ + last_bytes_read_);
    std::memcpy(data_.data() + size_, &tmp_buffer_[0], last_bytes_read_);
    size_ += last_bytes_read_;

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
