#pragma once

#include <atomic>
#include <queue>

#include <engine/ev/watcher.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl::io {

class SocketWriter final {
 public:
  SocketWriter(engine::ev::ThreadControl& thread, int fd);
  ~SocketWriter();

  // This function doesn't block and should only be called from ev thread
  void Write(const char* data, size_t size);

  void Stop();

 private:
  void StartWrite();
  static void OnEventWrite(struct ev_loop*, ev_io* io, int events) noexcept;

  class Buffer final {
   public:
    void Write(const char* data, size_t size);

    void Flush(int fd) noexcept;

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

    std::queue<Chunk> data_;
    std::atomic<size_t> size_{0};
  };

  engine::ev::Watcher<ev_io> watcher_;
  int fd_;

  Buffer buffer_;
};

}  // namespace urabbitmq::impl::io

USERVER_NAMESPACE_END
