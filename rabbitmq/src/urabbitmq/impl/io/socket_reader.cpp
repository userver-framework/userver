#include "socket_reader.hpp"

#include <sys/socket.h>
#include <cstring>

#include <urabbitmq/impl/amqp_connection_handler.hpp>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl::io {

SocketReader::SocketReader(AmqpConnectionHandler& parent,
                           engine::ev::ThreadControl& thread, int fd)
    : parent_{parent}, watcher_{thread, this}, fd_{fd} {
  watcher_.Init(&OnEventRead, fd_, EV_READ);
}

SocketReader::~SocketReader() { Stop(); }

void SocketReader::Start(AMQP::Connection* connection) {
  conn_ = connection;
  StartRead();
}

void SocketReader::Stop() { watcher_.Stop(); }

void SocketReader::StartRead() { watcher_.Start(); }

void SocketReader::OnEventRead(struct ev_loop*, ev_io* io,
                               int events) noexcept {
  auto* self = static_cast<SocketReader*>(io->data);
  self->watcher_.Stop();

  if (events & EV_READ) {
    if (!self->buffer_.Read(self->fd_, self->conn_)) {
      self->conn_->fail("socket error");
      self->parent_.Invalidate();
      return;
    }
    self->watcher_.Start();
  }
}

SocketReader::Buffer::Buffer() { data_.resize(kTmpBufferSize); }

bool SocketReader::Buffer::Read(int fd, AMQP::Connection* conn) {
  const auto read = ::recv(fd, &tmp_buffer_[0], kTmpBufferSize, 0);

  if (read < 0) {
    if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) return true;
    return false;
  }
  if (read == 0) {
    return false;
  }

  data_.resize(size_ + read);
  std::memcpy(data_.data() + size_, &tmp_buffer_[0], read);
  size_ += read;

  const auto parsed = conn->parse(data_.data(), size_);
  if (parsed != 0) {
    if (size_ != parsed) {
      int a = 5;
    }
    std::memmove(data_.data(), data_.data() + parsed, size_ - parsed);
    size_ -= parsed;
  }

  return true;
}

}  // namespace urabbitmq::impl::io

USERVER_NAMESPACE_END
