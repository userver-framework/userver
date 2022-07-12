#pragma once

#include <engine/ev/watcher.hpp>

#include <urabbitmq/impl/io/output_buffer.hpp>

namespace AMQP {
class Connection;
}

USERVER_NAMESPACE_BEGIN

namespace urabbitmq::impl {

class AmqpConnectionHandler;

namespace io {

class ISocket;

class SocketWriter final {
 public:
  SocketWriter(AmqpConnectionHandler& parent, engine::ev::ThreadControl& thread,
               ISocket& socket);
  ~SocketWriter();

  // This function doesn't block and should only be called from ev thread
  void Write(AMQP::Connection* conn, const char* data, size_t size);

  void Stop();

 private:
  void StartWrite();
  static void OnEventWrite(struct ev_loop*, ev_io* io, int events) noexcept;

  AmqpConnectionHandler& parent_;

  engine::ev::Watcher<ev_io> watcher_;
  ISocket& socket_;

  Buffer buffer_;
  AMQP::Connection* conn_{nullptr};
};

}  // namespace io

}  // namespace urabbitmq::impl

USERVER_NAMESPACE_END
