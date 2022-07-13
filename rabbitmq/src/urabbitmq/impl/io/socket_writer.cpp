#include "socket_writer.hpp"

#include <amqpcpp.h>

#include <urabbitmq/impl/amqp_connection_handler.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl::io {

SocketWriter::SocketWriter(AmqpConnectionHandler& parent, ISocket& socket)
    : parent_{parent}, socket_{socket} {
  w_.data = static_cast<void*>(this);
  ev_io_init(&w_, &OnEventWrite, socket_.GetFd(), EV_WRITE);
}

SocketWriter::~SocketWriter() { Stop(); }

void SocketWriter::Write(AMQP::Connection* conn, const char* data,
                         size_t size) {
  UASSERT(parent_.GetEvThread().IsInEvThread());

  conn_ = conn;
  if (size == 0) return;

  buffer_.Write(data, size);
  StartWrite();
}

void SocketWriter::Stop() {
  parent_.GetEvThread().RunInEvLoopSync(
      [this] { ev_io_stop(parent_.GetEvThread().GetEvLoop(), &w_); });
}

void SocketWriter::StartWrite() {
  ev_io_start(parent_.GetEvThread().GetEvLoop(), &w_);
}

void SocketWriter::OnEventWrite(struct ev_loop* loop, ev_io* io,
                                int events) noexcept {
  auto* self = static_cast<SocketWriter*>(io->data);
  ev_io_stop(loop, &self->w_);

  if (events & EV_WRITE) {
    if (!self->buffer_.Flush(self->socket_)) {
      self->conn_->fail("socket error");
      self->parent_.Invalidate();
      return;
    }
    if (self->buffer_.HasData()) {
      self->StartWrite();
    }
  }
}

}  // namespace urabbitmq::impl::io

USERVER_NAMESPACE_END