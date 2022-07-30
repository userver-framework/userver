#pragma once

#include <memory>

#include <engine/ev/thread_control.hpp>
#include <userver/clients/dns/resolver_fwd.hpp>
#include <userver/engine/io/socket.hpp>

#include <urabbitmq/impl/handler_state.hpp>
#include <urabbitmq/impl/io/isocket.hpp>
#include <urabbitmq/impl/io/socket_reader.hpp>
#include <urabbitmq/impl/io/socket_writer.hpp>

#include <amqpcpp.h>

USERVER_NAMESPACE_BEGIN

namespace urabbitmq {

struct EndpointInfo;
struct AuthSettings;

namespace impl {

class AmqpConnectionHandler final : public AMQP::ConnectionHandler {
 public:
  AmqpConnectionHandler(clients::dns::Resolver& resolver,
                        engine::ev::ThreadControl& thread,
                        const EndpointInfo& endpoint,
                        const AuthSettings& auth_settings, bool secure);
  ~AmqpConnectionHandler() override;

  engine::ev::ThreadControl& GetEvThread();

  void onProperties(AMQP::Connection* connection, const AMQP::Table& server,
                    AMQP::Table& client) override;

  void onData(AMQP::Connection* connection, const char* buffer,
              size_t size) override;

  void onError(AMQP::Connection* connection, const char* message) override;

  void onClosed(AMQP::Connection* connection) override;

  void OnConnectionCreated(AMQP::Connection* connection);
  void OnConnectionDestruction();

  void Invalidate();
  bool IsBroken() const;

  void AccountBufferFlush(size_t size);

  std::shared_ptr<HandlerState> GetState() const;

 private:
  engine::ev::ThreadControl thread_;

  std::unique_ptr<io::ISocket> socket_;
  io::SocketWriter writer_;
  io::SocketReader reader_;

  std::shared_ptr<HandlerState> state_;

  class WriteBufferFlowControl final {
   public:
    WriteBufferFlowControl(HandlerState& state);

    void AccountWrite(size_t size);
    void AccountFlush(size_t size);

   private:
    static constexpr size_t kFlowControlStartThreshold = 1 << 22;  // 4Mb
    static constexpr size_t kFlowControlStopThreshold =
        kFlowControlStartThreshold / 2;

    size_t buffer_size_{0};
    HandlerState& state_;
  };
  WriteBufferFlowControl flow_control_;
};

}  // namespace impl
}  // namespace urabbitmq

USERVER_NAMESPACE_END