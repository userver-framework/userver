#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <queue>
#include <stack>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

class AmqpConnectionHandler;

namespace io {

class ISocket;

class Chunk final {
 public:
  size_t Write(const char* data, size_t size);
  const char* Data() const;
  size_t Size() const;
  void Advance(size_t size);
  bool Full() const;

  void Reset();

 private:
  class Header final {
   public:
    size_t& Size();
    size_t Size() const;
    size_t& ReadOffset();
    size_t ReadOffset() const;

   private:
    size_t size_{0};
    size_t read_offset_{0};
  };

  static constexpr size_t kSize =
      (1 << 14) - sizeof(Header) - sizeof(std::uintptr_t);

  Header header_{};
  char data_[kSize]{};
};

/*template <size_t Size>
struct FailAssert final {
  static_assert(!Size);
};

FailAssert<sizeof(Chunk)> f;*/

class ChunkPool final {
 public:
  ChunkPool(size_t size);

  class ChunkHolder final {
   public:
    ChunkHolder(ChunkPool& pool, std::unique_ptr<Chunk>&& chunk);
    ~ChunkHolder();

    ChunkHolder(ChunkHolder&& other) noexcept;

    Chunk* operator->() const;

   private:
    ChunkPool* pool_;
    std::unique_ptr<Chunk> chunk_;
  };

  ChunkHolder Get();

  void Release(std::unique_ptr<Chunk>&& chunk);

 private:
  size_t max_size_;
  std::stack<std::unique_ptr<Chunk>> chunks_;
};

class Buffer final {
 public:
  Buffer(AmqpConnectionHandler& handler);

  void Write(const char* data, size_t size);

  bool Flush(ISocket& socket) noexcept;

  bool HasData() const noexcept;

  size_t SizeApprox() const;

 private:
  AmqpConnectionHandler& handler_;

  // The most common case is probably some small writes that get flushed
  // right away, so we pool chunks to not reallocate them too much
  ChunkPool pool_;
  std::queue<ChunkPool::ChunkHolder> data_;
  std::atomic<size_t> size_{0};
};

}  // namespace io

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
