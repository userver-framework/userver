#pragma once

#include <vector>

#include <engine/ev/watcher.hpp>

namespace AMQP {
class Connection;
}

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

class AmqpConnectionHandler;

namespace io {

class ISocket;

class SocketReader final {
 public:
  SocketReader(AmqpConnectionHandler& parent, ISocket& socket);
  ~SocketReader();

  void Start(AMQP::Connection* connection);

  void Stop();

 private:
  void StartRead();
  static void OnEventRead(struct ev_loop*, ev_io* io, int events) noexcept;

  class Buffer final {
   public:
    Buffer();

    bool Read(ISocket& socket, AMQP::Connection* conn,
              AmqpConnectionHandler& parent);

   private:
    static constexpr size_t kTmpBufferSize = 1 << 15;
    char tmp_buffer_[kTmpBufferSize]{};

    std::vector<char> data_{};
    size_t size_{0};
  };

  AmqpConnectionHandler& parent_;

  ev_io w_{};

  ISocket& socket_;

  Buffer buffer_;
  AMQP::Connection* conn_{nullptr};
};

}  // namespace io

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
