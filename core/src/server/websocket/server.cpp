#include <userver/server/websocket/server.hpp>

#include <userver/components/component.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include "protocol.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::websocket {

namespace {
inline void SendExactly(engine::io::WritableBase& writable,
                        utils::impl::Span<const char> data1,
                        utils::impl::Span<const char> data2) {
  if (writable.WriteAll(
          {{data1.data(), data1.size()}, {data2.data(), data2.size()}}, {}) !=
      data1.size() + data2.size())
    throw(engine::io::IoException() << "Socket closed during transfer");
}

using utils::impl::Span;

Message CloseMessage(CloseStatus status) { return {{}, status, false}; }

}  // namespace

Config Parse(const yaml_config::YamlConfig& config,
             formats::parse::To<Config>) {
  return {
      config["max-remote-payload"].As<unsigned>(65536),
      config["fragment-size"].As<unsigned>(65536),
  };
}

class WebSocketConnectionImpl final : public WebSocketConnection {
 public:
 private:
  std::unique_ptr<engine::io::RwBase> io;

  struct MessageExtended final {
    const Message& msg;
    bool ping = false;
    bool pong = false;
  };

  // write_mutex_ should be obtained for each write to the socket.
  // Two possible writers: reading coroutine with "PONG" response
  // and user coroutine with data response.
  engine::Mutex write_mutex_;

  const engine::io::Sockaddr remote_addr_;
  Statistics stats_;

  // It is safe to have a global per-socket parser state as there is
  // only a single task calling Recv().
  impl::FrameParserState frame_;

  Config config;

 public:
  WebSocketConnectionImpl(std::unique_ptr<engine::io::RwBase> io_,
                          const engine::io::Sockaddr& remote_addr,
                          const Config& server_config)
      : io(std::move(io_)), remote_addr_(remote_addr), config(server_config) {}

  ~WebSocketConnectionImpl() override {
    LOG_TRACE() << "Websocket connection closed";
  }

  void SendExtended(MessageExtended& message) {
    stats_.msg_sent++;
    stats_.bytes_sent += message.msg.data.size();

    std::unique_lock lock(write_mutex_);

    LOG_TRACE() << "Write message " << message.msg.data.size() << " bytes";
    if (message.ping) {
      SendExactly(*io, impl::frames::PingFrame(), {});
    } else if (message.pong) {
      SendExactly(*io,
                  impl::frames::MakeControlFrame(impl::WSOpcodes::kPong,
                                                 message.msg.data),
                  message.msg.data);
    } else if (message.msg.close_status.has_value()) {
      SendExactly(*io,
                  impl::frames::CloseFrame(
                      static_cast<int>(message.msg.close_status.value())),
                  {});
    } else if (!message.msg.data.empty()) {
      Span<const char> dataToSend{message.msg.data};
      auto continuation = impl::frames::Continuation::kNo;
      while (dataToSend.size() > config.fragment_size &&
             config.fragment_size > 0) {
        SendExactly(
            *io,
            impl::frames::DataFrameHeader(
                dataToSend.first(config.fragment_size), message.msg.is_text,
                continuation, impl::frames::Final::kNo),
            dataToSend.first(config.fragment_size));
        continuation = impl::frames::Continuation::kYes;
        dataToSend = dataToSend.last(dataToSend.size() - config.fragment_size);
      }
      SendExactly(*io,
                  impl::frames::DataFrameHeader(dataToSend, message.msg.is_text,
                                                continuation,
                                                impl::frames::Final::kYes),
                  dataToSend);
    }
  }

  void Send(const Message& message) override {
    MessageExtended mext{message, false, false};
    SendExtended(mext);
  }

  void Recv(Message& msg) override {
    msg.data.resize(0);  // do not call .clear() to keep the allocated memory
    frame_.payload = &msg.data;

    try {
      while (true) {
        // ReadWSFrame() returns kGoingAway in case of task cancellation
        CloseStatus status_raw =
            ReadWSFrame(frame_, *io, config.max_remote_payload);

        auto status = static_cast<CloseStatusInt>(status_raw);
        LOG_TRACE() << fmt::format(
            "Read frame is_text {}, closed {}, data size {} status {} "
            "waitCont {}",
            frame_.is_text, frame_.closed, frame_.payload->size(), status,
            frame_.waiting_continuation);
        if (status != 0) {
          Message msg;
          msg.close_status = status_raw;
          // msg.closed is not used
          MessageExtended close_msg{msg, false, false};
          SendExtended(close_msg);
          msg = CloseMessage(status_raw);
          return;
        }

        if (frame_.closed) {
          msg = CloseMessage(
              static_cast<CloseStatus>(frame_.remote_close_status));
          return;
        }

        if (frame_.ping_received) {
          LOG_ERROR() << "Data size " << frame_.payload->size();
          Message pong_msg;
          pong_msg.data = std::move(*frame_.payload);
          MessageExtended pongMsg{pong_msg, false, true};
          SendExtended(pongMsg);
          frame_.ping_received = false;
          continue;
        }
        if (frame_.pong_received) {
          frame_.pong_received = false;
          continue;
        }
        if (frame_.waiting_continuation) continue;

        msg.is_text = frame_.is_text;
        stats_.msg_recv++;
        stats_.bytes_recv += msg.data.size();
        return;
      }
    } catch (std::exception const& e) {
      LOG_TRACE() << "Exception during frame_ parsing " << e;
      throw;
    }
  }

  void Close(CloseStatus status_code) override {
    Send(CloseMessage(status_code));
  }

  const engine::io::Sockaddr& RemoteAddr() const override {
    return remote_addr_;
  }

  void AddFinalTags(tracing::Span& span) const override {
    span.AddTag("peer", remote_addr_.PrimaryAddressString());
    span.AddTag("msg_sent", stats_.msg_sent.load());
    span.AddTag("msg_recv", stats_.msg_recv.load());
    span.AddTag("bytes_sent", stats_.bytes_sent.load());
    span.AddTag("bytes_recv", stats_.bytes_recv.load());
  }

  void AddStatistics(Statistics& stats) const override {
    stats.msg_sent += stats_.msg_sent;
    stats.msg_recv += stats_.msg_recv;
    stats.bytes_sent += stats_.bytes_sent;
    stats.bytes_recv += stats_.bytes_recv;
  }
};

WebSocketConnection::WebSocketConnection() = default;

WebSocketConnection::~WebSocketConnection() = default;

std::shared_ptr<WebSocketConnection> MakeWebSocket(
    std::unique_ptr<engine::io::RwBase>&& socket,
    engine::io::Sockaddr&& peer_name, const Config& config) {
  return std::make_shared<WebSocketConnectionImpl>(
      std::move(socket), std::move(peer_name), config);
}

}  // namespace server::websocket

USERVER_NAMESPACE_END
