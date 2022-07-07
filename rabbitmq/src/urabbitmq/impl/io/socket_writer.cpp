#include "socket_writer.hpp"

#include <cstring>
#include <sys/socket.h>

#include <engine/ev/thread_control.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl::io {

SocketWriter::SocketWriter(engine::ev::ThreadControl& thread, int fd)
: watcher_{thread, this}, fd_{fd} {
  watcher_.Init(&OnEventWrite, fd_, EV_WRITE);
}

SocketWriter::~SocketWriter() { Stop(); }

void SocketWriter::Write(const char* data, size_t size) {
  if (data == 0) return;

  buffer_.Write(data, size);
  StartWrite();
}

size_t SocketWriter::Buffer::Chunk::Write(const char* data, size_t size) {
  const auto available = kSize - size_;
  if (available != 0) {
    const auto write = std::min(available, size);
    std::memcpy(&data_[size_], data, write);
    size_ += write;

    return write;
  }

  return 0;
}

void SocketWriter::Stop() {
  watcher_.Stop();
}

const char* SocketWriter::Buffer::Chunk::Data() const {
  if (read_offset_ == kSize) return &data_[0];

  return &data_[read_offset_];
}

size_t SocketWriter::Buffer::Chunk::Size() const {
  return size_ - read_offset_;
}

void SocketWriter::Buffer::Chunk::Advance(size_t size) {
  read_offset_ += size;
}

bool SocketWriter::Buffer::Chunk::Full() const {
  return size_ == kSize;
}

void SocketWriter::Buffer::Write(const char* data, size_t size) {
  if (size == 0) return;

  if (data_.empty()) {
    data_.emplace<Chunk>({});
  }

  size_t written = 0;
  while (written < size) {
    const auto written_to_chunk = data_.back().Write(data + written, size - written);
    if (written_to_chunk == 0) {
      data_.emplace<Chunk>({});
    }

    written += written_to_chunk;
  }
}

void SocketWriter::StartWrite() {
  watcher_.Start();
}

void SocketWriter::OnEventWrite(struct ev_loop*, ev_io* io, int events) noexcept {
  auto* self = static_cast<SocketWriter*>(io->data);
  self->watcher_.Stop();

  if (events & EV_WRITE) {
    self->buffer_.Flush(self->fd_);
    if (self->buffer_.HasData()) {
      self->StartWrite();
    }
  }
}

void SocketWriter::Buffer::Flush(int fd) noexcept {
  while (!data_.empty()) {
    const auto size = data_.front().Size();
    if (size == 0) {
      if (data_.front().Full()) {
        data_.pop();
        continue;
      } else {
        break;
      }
    }

    const char* data = data_.front().Data();
    const auto sent = ::send(fd, data, size,
// MAC_COMPAT: does not support MSG_NOSIGNAL
#ifdef MSG_NOSIGNAL
                             MSG_NOSIGNAL |
#endif
                                 0);
    if (sent < 0) {
      // TODO : socket is broken, recover somehow
      abort();
    }
    if (sent == 0) break;
    data_.front().Advance(static_cast<size_t>(sent));
  }
}

bool SocketWriter::Buffer::HasData() const noexcept {
  return !data_.empty() && data_.front().Size() != 0;
}

}

USERVER_NAMESPACE_END