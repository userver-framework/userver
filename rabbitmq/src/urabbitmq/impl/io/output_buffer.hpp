#pragma once

#include <atomic>
#include <queue>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

class AmqpConnectionHandler;

namespace io {

class ISocket;

class Buffer final {
 public:
  Buffer(AmqpConnectionHandler& handler);

  void Write(const char* data, size_t size);

  bool Flush(ISocket& socket) noexcept;

  bool HasData() const noexcept;

  size_t SizeApprox() const;

 private:
  class Chunk final {
   public:
    size_t Write(const char* data, size_t size);
    const char* Data() const;
    size_t Size() const;
    void Advance(size_t size);
    bool Full() const;

   private:
    static constexpr size_t kSize = 1 << 14;

    char data_[kSize]{};
    size_t size_{0};
    size_t read_offset_{0};
  };

  AmqpConnectionHandler& handler_;
  std::queue<Chunk> data_;
  std::atomic<size_t> size_{0};
};

}  // namespace io

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
