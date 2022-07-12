#include "socket_writer.hpp"

#include <amqpcpp.h>

#include <urabbitmq/impl/amqp_connection_handler.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl::io {

SocketWriter::SocketWriter(AmqpConnectionHandler& parent,
                           engine::ev::ThreadControl& thread, ISocket& socket)
    : parent_{parent}, watcher_{thread, this}, socket_{socket} {
  watcher_.Init(&OnEventWrite, socket_.GetFd(), EV_WRITE);
}

SocketWriter::~SocketWriter() { Stop(); }

void SocketWriter::Write(AMQP::Connection* conn, const char* data,
                         size_t size) {
  conn_ = conn;
  if (size == 0) return;

  buffer_.Write(data, size);
  StartWrite();
}

void SocketWriter::Stop() { watcher_.Stop(); }

void SocketWriter::StartWrite() { watcher_.Start(); }

void SocketWriter::OnEventWrite(struct ev_loop*, ev_io* io,
                                int events) noexcept {
  auto* self = static_cast<SocketWriter*>(io->data);
  self->watcher_.Stop();

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