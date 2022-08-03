#include "socket_reader.hpp"

#include <cstring>

#include <urabbitmq/impl/amqp_connection_handler.hpp>
#include <urabbitmq/impl/io/isocket.hpp>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl::io {

SocketReader::SocketReader(AmqpConnectionHandler& parent, ISocket& socket)
    : parent_{parent}, socket_{socket} {
  w_.data = static_cast<void*>(this);
  ev_io_init(&w_, &OnEventRead, socket_.GetFd(), EV_READ);
}

SocketReader::~SocketReader() { Stop(); }

void SocketReader::Start(AMQP::Connection* connection) {
  UASSERT(parent_.GetEvThread().IsInEvThread());

  conn_ = connection;
  StartRead();
}

void SocketReader::Stop() {
  parent_.GetEvThread().RunInEvLoopSync(
      [this] { ev_io_stop(parent_.GetEvThread().GetEvLoop(), &w_); });
}

void SocketReader::StartRead() {
  ev_io_start(parent_.GetEvThread().GetEvLoop(), &w_);
}

void SocketReader::OnEventRead(struct ev_loop* loop, ev_io* io,
                               int events) noexcept {
  auto* self = static_cast<SocketReader*>(io->data);
  ev_io_stop(loop, &self->w_);

  if (events & EV_READ) {
    if (!self->buffer_.Read(self->socket_, self->conn_, self->parent_)) {
      self->conn_->fail("socket error");
      self->parent_.Invalidate();
      return;
    }
    self->StartRead();
  }
}

SocketReader::Buffer::Buffer() { data_.resize(kTmpBufferSize); }

bool SocketReader::Buffer::Read(ISocket& socket, AMQP::Connection* conn,
                                AmqpConnectionHandler& parent) {
  try {
    const auto read = socket.Read(&tmp_buffer_[0], kTmpBufferSize);
    if (read == 0) {
      return false;
    }

    data_.resize(size_ + read);
    std::memcpy(data_.data() + size_, &tmp_buffer_[0], read);
    size_ += read;

    const auto parsed = conn->parse(data_.data(), size_);
    if (parsed != 0) {
      std::memmove(data_.data(), data_.data() + parsed, size_ - parsed);
      size_ -= parsed;
      parent.AccountRead(parsed);
    }

    return true;
  } catch (const engine::io::IoWouldBlockException&) {
    return true;
  } catch (const std::exception&) {
    return false;
  }
}

}  // namespace urabbitmq::impl::io

USERVER_NAMESPACE_END
