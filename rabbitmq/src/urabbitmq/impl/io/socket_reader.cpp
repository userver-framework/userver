#include "socket_reader.hpp"

#include <cstring>
#include <sys/socket.h>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl::io {

SocketReader::SocketReader(engine::ev::ThreadControl& thread, int fd)
  : watcher_{thread, this}, fd_{fd} {
  watcher_.Init(&OnEventRead, fd_, EV_READ);
}

SocketReader::~SocketReader() { Stop(); }

void SocketReader::Start(AMQP::Connection* connection) {
  conn_ = connection;
  StartRead();
}

void SocketReader::Stop() {
  watcher_.Stop();
}

void SocketReader::StartRead() {
  watcher_.Start();
}

void SocketReader::OnEventRead(struct ev_loop*, ev_io* io, int events) noexcept {
  auto* self = static_cast<SocketReader*>(io->data);
  self->watcher_.Stop();

  if (events & EV_READ) {
    self->buffer_.Read(self->fd_, self->conn_);
    self->watcher_.Start();
  }
}

SocketReader::Buffer::Buffer() {
  data_.reserve(kTmpBufferSize);
}

void SocketReader::Buffer::Read(int fd, AMQP::Connection* conn) {
  const auto read = ::recv(fd, &tmp_buffer_[0], kTmpBufferSize, 0);

  if (read < 0) {
    // TODO : socket got broken, notify the caller
    abort();
  }
  if (read == 0) {
    // TODO : what is this case actually?
    abort();
  }

  data_.reserve(size_ + read);
  std::memcpy(data_.data() + size_, &tmp_buffer_[0], read);
  size_ += read;

  const auto parsed = conn->parse(data_.data(), size_);
  if (parsed != 0) {
    std::memmove(data_.data(), data_.data() + parsed, size_ - parsed);
    size_ -= parsed;
  }
}

}

USERVER_NAMESPACE_END
