#include "output_buffer.hpp"

#include <cstring>

#include <urabbitmq/impl/amqp_connection_handler.hpp>
#include <urabbitmq/impl/io/isocket.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl::io {

size_t Chunk::Write(const char* data, size_t size) {
  const auto available = kSize - header_.Size();
  if (available != 0) {
    const auto write = std::min(available, size);
    std::memcpy(&data_[header_.Size()], data, write);
    header_.Size() += write;

    return write;
  }

  return 0;
}

const char* Chunk::Data() const {
  if (header_.ReadOffset() == kSize) return &data_[0];

  return &data_[header_.ReadOffset()];
}

size_t Chunk::Size() const { return header_.Size() - header_.ReadOffset(); }

void Chunk::Advance(size_t size) { header_.ReadOffset() += size; }

bool Chunk::Full() const { return header_.Size() == kSize; }

void Chunk::Reset() {
  header_.Size() = 0;
  header_.ReadOffset() = 0;
}

size_t& Chunk::Header::Size() { return size_; }

size_t Chunk::Header::Size() const { return size_; }

size_t& Chunk::Header::ReadOffset() { return read_offset_; }

size_t Chunk::Header::ReadOffset() const { return read_offset_; }

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
  // We yield writes to leave a room for other operations
  constexpr size_t kMaxSentInOneGo = 1 << 20;
  size_t sent_in_one_go = 0;

  while (!data_.empty() && sent_in_one_go < kMaxSentInOneGo) {
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
      sent_in_one_go += sent;
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