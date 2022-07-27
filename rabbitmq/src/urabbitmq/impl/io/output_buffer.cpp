#include "output_buffer.hpp"

#include <cstring>

#include <urabbitmq/impl/amqp_connection_handler.hpp>
#include <urabbitmq/impl/io/isocket.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl::io {

size_t Chunk::Write(const char* data, size_t size) {
  const auto available = kSize - size_;
  if (available != 0) {
    const auto write = std::min(available, size);
    std::memcpy(&data_[size_], data, write);
    size_ += write;

    return write;
  }

  return 0;
}

const char* Chunk::Data() const {
  if (read_offset_ == kSize) return &data_[0];

  return &data_[read_offset_];
}

size_t Chunk::Size() const { return size_ - read_offset_; }

void Chunk::Advance(size_t size) { read_offset_ += size; }

bool Chunk::Full() const { return size_ == kSize; }

void Chunk::Reset() {
  size_ = 0;
  read_offset_ = 0;
}

ChunkPool::ChunkPool(size_t size) : max_size_{size} {
  for (size_t i = 0; i < max_size_; ++i) {
    chunks_.push(std::make_unique<Chunk>());
  }
}

ChunkPool::ChunkHolder::ChunkHolder(ChunkPool& pool,
                                    std::unique_ptr<Chunk>&& chunk)
    : pool_{&pool}, chunk_{std::move(chunk)} {}

ChunkPool::ChunkHolder::ChunkHolder(ChunkHolder&& other) noexcept = default;

ChunkPool::ChunkHolder::~ChunkHolder() {
  if (chunk_) {
    pool_->Release(std::move(chunk_));
  }
}

Chunk* ChunkPool::ChunkHolder::operator->() const { return chunk_.get(); }

ChunkPool::ChunkHolder ChunkPool::Get() {
  if (chunks_.empty()) {
    return {*this, std::make_unique<Chunk>()};
  } else {
    ChunkHolder result{*this, std::move(chunks_.top())};
    chunks_.pop();
    return result;
  }
}

void ChunkPool::Release(std::unique_ptr<Chunk>&& chunk) {
  if (chunks_.size() < max_size_) {
    chunk->Reset();
    chunks_.push(std::move(chunk));
  }
}

Buffer::Buffer(AmqpConnectionHandler& handler)
    : handler_{handler}, pool_{100} {}

void Buffer::Write(const char* data, size_t size) {
  if (size == 0) return;

  if (data_.empty()) {
    data_.push(pool_.Get());
  }

  size_t written = 0;
  while (written < size) {
    const auto written_to_chunk =
        data_.back()->Write(data + written, size - written);
    if (written_to_chunk == 0) {
      data_.push(pool_.Get());
    }

    written += written_to_chunk;
  }

  size_.fetch_add(size, std::memory_order_relaxed);
}

bool Buffer::Flush(ISocket& socket) noexcept {
  while (!data_.empty()) {
    const auto size = data_.front()->Size();
    if (size == 0) {
      if (data_.front()->Full()) {
        data_.pop();
        continue;
      } else {
        break;
      }
    }

    const char* data = data_.front()->Data();
    try {
      const auto sent = socket.Write(data, size);
      if (sent == 0) {
        return false;
      }

      handler_.AccountBufferFlush(sent);
      data_.front()->Advance(sent);
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
  return !data_.empty() && data_.front()->Size() != 0;
}

size_t Buffer::SizeApprox() const { return size_; }

}  // namespace urabbitmq::impl::io

USERVER_NAMESPACE_END