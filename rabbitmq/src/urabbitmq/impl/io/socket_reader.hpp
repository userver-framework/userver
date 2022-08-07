#pragma once

#include <vector>

#include <userver/engine/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

class AmqpConnectionHandler;
class AmqpConnection;

namespace io {

class ISocket;

class SocketReader final {
 public:
  SocketReader(AmqpConnectionHandler& parent, ISocket& socket);
  ~SocketReader();

  void Start(AmqpConnection* connection);

  void Stop();

 private:
  class Buffer final {
   public:
    Buffer();

    bool Read(ISocket& socket, AmqpConnection* conn,
              AmqpConnectionHandler& parent);

   private:
    static constexpr size_t kTmpBufferSize = 1 << 15;
    char tmp_buffer_[kTmpBufferSize]{};

    std::vector<char> data_{};
    size_t size_{0};
  };

  AmqpConnectionHandler& parent_;
  ISocket& socket_;

  Buffer buffer_;
  AmqpConnection* conn_{nullptr};

  engine::TaskWithResult<void> reader_task_;
};

}  // namespace io

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
