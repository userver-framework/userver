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
                        utils::span<const char> data1,
                        utils::span<const std::byte> data2) {
  if (writable.WriteAll(
          {{data1.data(), data1.size()}, {data2.data(), data2.size()}}, {}) !=
      data1.size() + data2.size())
    throw(engine::io::IoException() << "Socket closed during transfer");
}

Message CloseMessage(CloseStatus status) { return {{}, status, false}; }

utils::span<const std::byte> MakeBinarySpan(utils::span<const char> span) {
  return utils::as_bytes(span);
}

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
    utils::span<const std::byte> data;
    impl::WSOpcodes opcode{};
    std::optional<CloseStatus> close_status;
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
    stats_.bytes_sent += message.data.size();

    const std::unique_lock lock(write_mutex_);

    LOG_TRACE() << "Write message " << message.data.size() << " bytes";
    if (message.opcode == impl::WSOpcodes::kPing) {
      SendExactly(*io, impl::frames::PingFrame(), {});
    } else if (message.opcode == impl::WSOpcodes::kPong) {
      const auto control_frame =
          impl::frames::MakeControlFrame(impl::WSOpcodes::kPong, message.data);
      SendExactly(*io, control_frame, message.data);
    } else if (message.close_status.has_value()) {
      const auto close_frame = impl::frames::CloseFrame(
          static_cast<int>(message.close_status.value()));
      SendExactly(*io, close_frame, {});
    } else if (!message.data.empty()) {
      utils::span<const std::byte> data_to_send{message.data};
      auto continuation = impl::frames::Continuation::kNo;
      while (data_to_send.size() > config.fragment_size &&
             config.fragment_size > 0) {
        const auto data_frame_header = impl::frames::DataFrameHeader(
            data_to_send.first(config.fragment_size),
            message.opcode == impl::WSOpcodes::kText, continuation,
            impl::frames::Final::kNo);
        SendExactly(*io, data_frame_header,
                    data_to_send.first(config.fragment_size));
        continuation = impl::frames::Continuation::kYes;
        data_to_send =
            data_to_send.last(data_to_send.size() - config.fragment_size);
      }
      const auto data_frame_header = impl::frames::DataFrameHeader(
          data_to_send, message.opcode == impl::WSOpcodes::kText, continuation,
          impl::frames::Final::kYes);
      SendExactly(*io, data_frame_header, data_to_send);
    }
  }

  void Send(const Message& message) override {
    MessageExtended mext{
        MakeBinarySpan(message.data),
        message.is_text ? impl::WSOpcodes::kText : impl::WSOpcodes::kBinary,
        message.close_status};
    SendExtended(mext);
  }

  void SendText(std::string_view message) override {
    MessageExtended mext{MakeBinarySpan(message), impl::WSOpcodes::kText, {}};
    SendExtended(mext);
  }

  void DoSendBinary(utils::span<const std::byte> message) override {
    MessageExtended mext{message, impl::WSOpcodes::kBinary, {}};
    SendExtended(mext);
  }

  void Recv(Message& msg) override {
    msg.data.resize(0);  // do not call .clear() to keep the allocated memory
    frame_.payload = &msg.data;

    try {
      while (true) {
        size_t payload_len = 0;
        // ReadWSFrame() returns kGoingAway in case of task cancellation
        CloseStatus status_raw =
            ReadWSFrame(frame_, *io, config.max_remote_payload, payload_len);

        auto status = static_cast<CloseStatusInt>(status_raw);
        LOG_TRACE() << fmt::format(
            "Read frame is_text {}, closed {}, data size {} status {} "
            "waitCont {}",
            frame_.is_text, frame_.closed, frame_.payload->size(), status,
            frame_.waiting_continuation);
        if (status != 0) {
          MessageExtended close_msg{{}, impl::WSOpcodes::kClose, status_raw};
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
          MessageExtended pongMsg{
              MakeBinarySpan(*frame_.payload), impl::WSOpcodes::kPong, {}};
          SendExtended(pongMsg);
          frame_.payload->resize(frame_.payload->size() - payload_len);
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
