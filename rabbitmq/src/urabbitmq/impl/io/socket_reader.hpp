#pragma once

#include <vector>

#include <engine/ev/watcher.hpp>

namespace AMQP {
class Connection;
}

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl::io {

class SocketReader final {
 public:
  SocketReader(engine::ev::ThreadControl& thread, int fd);
  ~SocketReader();

  void Start(AMQP::Connection* connection);

  void Stop();
 private:
  void StartRead();
  static void OnEventRead(struct ev_loop*, ev_io* io, int events) noexcept;

  class Buffer final {
   public:
    Buffer();

    void Read(int fd, AMQP::Connection* conn);
   private:
    static constexpr size_t kTmpBufferSize = 1 << 14;
    char tmp_buffer_[kTmpBufferSize]{};

    std::vector<char> data_{};
    size_t size_{0};
  };

  engine::ev::Watcher<ev_io> watcher_;
  int fd_;

  Buffer buffer_;

  AMQP::Connection* conn_{nullptr};
};

}

USERVER_NAMESPACE_END
