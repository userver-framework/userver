#include "output_buffer.hpp"

#include <sys/socket.h>
#include <cerrno>
#include <cstring>

#include <urabbitmq/impl/amqp_connection_handler.hpp>
#include <urabbitmq/impl/io/isocket.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl::io {

Buffer::Buffer(AmqpConnectionHandler& handler) : handler_{handler} {}

void Buffer::Write(const char* data, size_t size) {
  if (size == 0) return;

  if (data_.empty()) {
    data_.emplace<Chunk>({});
  }

  size_t written = 0;
  while (written < size) {
    const auto written_to_chunk =
        data_.back().Write(data + written, size - written);
    if (written_to_chunk == 0) {
      data_.emplace<Chunk>({});
    }

    written += written_to_chunk;
  }

  size_.fetch_add(size, std::memory_order_relaxed);
}

bool Buffer::Flush(ISocket& socket) noexcept {
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
    try {
      const auto sent = socket.Write(data, size);
      if (sent == 0) {
        return false;
      }

      handler_.AccountBufferFlush(sent);
      data_.front().Advance(sent);
      size_.fetch_sub(sent);
    } catch (const engine::io::IoWouldBlockException&) {
      return true;
    } catch (const std::exception&) {
      return false;
    }
  }

  return true;
}

bool Buffer::HasData() const noexcept {
  return !data_.empty() && data_.front().Size() != 0;
}

size_t Buffer::SizeApprox() const { return size_; }

size_t Buffer::Chunk::Write(const char* data, size_t size) {
  const auto available = kSize - size_;
  if (available != 0) {
    const auto write = std::min(available, size);
    std::memcpy(&data_[size_], data, write);
    size_ += write;

    return write;
  }

  return 0;
}

const char* Buffer::Chunk::Data() const {
  if (read_offset_ == kSize) return &data_[0];

  return &data_[read_offset_];
}

size_t Buffer::Chunk::Size() const { return size_ - read_offset_; }

void Buffer::Chunk::Advance(size_t size) { read_offset_ += size; }

bool Buffer::Chunk::Full() const { return size_ == kSize; }

}  // namespace urabbitmq::impl::io

USERVER_NAMESPACE_END