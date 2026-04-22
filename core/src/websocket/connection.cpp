#include <userver/websocket/connection.hpp>

#include <atomic>

#include <boost/endian/conversion.hpp>

#include <userver/components/component.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/async.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <userver/websocket/impl/protocol.hpp>

USERVER_NAMESPACE_BEGIN

namespace websocket {

namespace {

template <typename... Spans>
void SendExactly(engine::io::WritableBase& writable, Spans... spans) {
    const size_t total_size = (spans.size() + ...);
    if (writable.WriteAll({engine::io::IoData{spans.data(), spans.size()}...}, {}) != total_size) {
        throw(engine::io::IoException() << "Socket closed during transfer");
    }
}

Message CloseMessage(CloseStatus status) { return {{}, status, false}; }

utils::span<const std::byte> MakeBinarySpan(utils::span<const char> span) { return utils::as_bytes(span); }

void SendFrame(
    engine::io::WritableBase& writable,
    utils::span<const char> header,
    utils::span<const std::byte> payload,
    impl::frames::Masked need_mask
) {
    if (need_mask == impl::frames::Masked::kNo) {
        SendExactly(writable, header, payload);
        return;
    }

    // Need masking: copy payload, apply mask, send
    std::vector<std::byte> payload_copy(payload.begin(), payload.end());

    const auto mask = impl::frames::Mask32::Generate();
    impl::frames::ApplyMask(payload_copy, mask);
    SendExactly(writable, header, utils::span{reinterpret_cast<const char*>(mask.mask8), 4}, utils::span{payload_copy});
}

}  // namespace

Config Parse(const yaml_config::YamlConfig& config, formats::parse::To<Config>) {
    return {
        .max_remote_payload = config["max-remote-payload"].As<unsigned>(65536),
        .fragment_size = config["fragment-size"].As<unsigned>(65536),
    };
}

class WebSocketConnectionImpl final : public WebSocketConnection {
public:
private:
    std::unique_ptr<engine::io::RwBase> io_;

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

    Config config_;

    std::atomic<std::size_t> ping_pending_count_{0};

    const impl::frames::Masked need_data_masking_;

public:
    WebSocketConnectionImpl(
        std::unique_ptr<engine::io::RwBase> io,
        const engine::io::Sockaddr& remote_addr,
        const Config& config,
        impl::frames::Masked need_data_masking
    )
        : io_(std::move(io)),
          remote_addr_(remote_addr),
          config_(config),
          need_data_masking_(need_data_masking)
    {}

    ~WebSocketConnectionImpl() override { LOG_TRACE() << "Websocket connection closed"; }

    void SendExtended(MessageExtended& message) {
        stats_.msg_sent++;
        stats_.bytes_sent += message.data.size();

        const std::lock_guard lock(write_mutex_);

        LOG_TRACE() << "Write message " << message.data.size() << " bytes";
        if (message.opcode == impl::WSOpcodes::kPing) {
            const auto frame = impl::frames::MakeControlFrame(impl::WSOpcodes::kPing, {}, need_data_masking_);

            SendFrame(*io_, frame, {}, need_data_masking_);
        } else if (message.opcode == impl::WSOpcodes::kPong) {
            const auto frame = impl::frames::MakeControlFrame(impl::WSOpcodes::kPong, message.data, need_data_masking_);

            SendFrame(*io_, frame, message.data, need_data_masking_);
        } else if (message.close_status.has_value()) {
            // Prepare payload (status code in big endian)
            auto status_be = boost::endian::native_to_big(static_cast<CloseStatusInt>(message.close_status.value()));
            auto payload = utils::span{reinterpret_cast<const std::byte*>(&status_be), sizeof(status_be)};

            const auto frame = impl::frames::MakeControlFrame(impl::WSOpcodes::kClose, payload, need_data_masking_);

            SendFrame(*io_, frame, payload, need_data_masking_);
        } else if (!message.data.empty()) {
            auto continuation = impl::frames::Continuation::kNo;
            utils::span<const std::byte> data_to_send{message.data};

            // Send fragments
            while (data_to_send.size() > config_.fragment_size && config_.fragment_size > 0) {
                const auto fragment = data_to_send.first(config_.fragment_size);
                const auto frame = impl::frames::DataFrameHeader(
                    fragment,
                    message.opcode == impl::WSOpcodes::kText,
                    continuation,
                    impl::frames::Final::kNo,
                    need_data_masking_
                );

                SendFrame(*io_, {frame.data(), frame.size()}, fragment, need_data_masking_);
                continuation = impl::frames::Continuation::kYes;
                data_to_send = data_to_send.last(data_to_send.size() - config_.fragment_size);
            }

            // Send final fragment
            const auto frame = impl::frames::DataFrameHeader(
                data_to_send,
                message.opcode == impl::WSOpcodes::kText,
                continuation,
                impl::frames::Final::kYes,
                need_data_masking_
            );
            SendFrame(*io_, {frame.data(), frame.size()}, data_to_send, need_data_masking_);
        }
    }

    void Send(const Message& message) override {
        MessageExtended mext{
            MakeBinarySpan(message.data),
            message.is_text ? impl::WSOpcodes::kText : impl::WSOpcodes::kBinary,
            message.close_status
        };
        SendExtended(mext);
    }

    void SendText(std::string_view message) override {
        MessageExtended mext{MakeBinarySpan(message), impl::WSOpcodes::kText, {}};
        SendExtended(mext);
    }

    void SendPing() override {
        MessageExtended ping_msg{{}, impl::WSOpcodes::kPing, {}};
        SendExtended(ping_msg);
        LOG_TRACE() << "Sent keep-alive ping";
        ping_pending_count_.fetch_add(1);
    }

    std::size_t NotAnsweredSequentialPingsCount() override { return ping_pending_count_.load(); }

    void DoSendBinary(utils::span<const std::byte> message) override {
        MessageExtended mext{message, impl::WSOpcodes::kBinary, {}};
        SendExtended(mext);
    }

    bool RecvImpl(Message& msg, bool do_not_wait_for_message_header) {
        msg.data.resize(0);  // do not call .clear() to keep the allocated memory
        frame_.payload = &msg.data;

        while (true) {
            std::size_t payload_len = 0;
            CloseStatus status_raw{};

            if (do_not_wait_for_message_header) {
                const auto opt_status_raw =
                    ReadWSFrameDontWaitForHeader(frame_, *io_, config_.max_remote_payload, payload_len);
                if (!opt_status_raw) {
                    return false;
                }
                status_raw = *opt_status_raw;
            } else {
                // ReadWSFrame() returns kGoingAway in case of task cancellation
                status_raw = ReadWSFrame(frame_, *io_, config_.max_remote_payload, payload_len);
            }

            const auto status = static_cast<CloseStatusInt>(status_raw);
            LOG_TRACE() << fmt::format(
                "Read frame is_text {}, closed {}, data size {} status {} "
                "waitCont {}",
                frame_.is_text,
                frame_.closed,
                frame_.payload->size(),
                status,
                frame_.waiting_continuation
            );

            if (status != 0) {
                MessageExtended close_msg{{}, impl::WSOpcodes::kClose, status_raw};
                SendExtended(close_msg);
                msg = CloseMessage(status_raw);
                return true;
            }

            if (frame_.closed) {
                msg = CloseMessage(static_cast<CloseStatus>(frame_.remote_close_status));
                return true;
            }

            if (frame_.ping_received) {
                MessageExtended pong_msg{MakeBinarySpan(*frame_.payload), impl::WSOpcodes::kPong, {}};
                SendExtended(pong_msg);
                frame_.payload->resize(frame_.payload->size() - payload_len);
                frame_.ping_received = false;
                continue;
            }
            if (frame_.pong_received) {
                frame_.pong_received = false;
                ping_pending_count_ = 0;
                continue;
            }
            if (frame_.waiting_continuation) {
                continue;
            }

            msg.is_text = frame_.is_text;
            stats_.msg_recv++;
            stats_.bytes_recv += msg.data.size();
            return true;
        }
    }

    void Recv(Message& msg) override {
        const auto ok = RecvImpl(msg, /*do_not_wait_for_message_header*/ false);
        UASSERT(ok);
    }

    // Aware! we can't drop the msg's buffer so for data sending we need to yet
    // another message.
    bool TryRecv(Message& msg) override { return RecvImpl(msg, /*do_not_wait_for_message_header*/ true); }

    void Close(CloseStatus status_code) override { Send(CloseMessage(status_code)); }

    const engine::io::Sockaddr& RemoteAddr() const override { return remote_addr_; }

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

    engine::io::ReadAwaiter& ReadAwaiter() override { return io_->GetReadableBase(); }

    engine::io::WriteAwaiter& WriteAwaiter() override { return io_->GetWritableBase(); }
};

WebSocketConnection::WebSocketConnection() = default;

WebSocketConnection::~WebSocketConnection() = default;

std::shared_ptr<WebSocketConnection> MakeServerWebSocketConnection(
    std::unique_ptr<engine::io::RwBase>&& socket,
    engine::io::Sockaddr&& peer_name,
    const Config& config
) {
    return std::make_shared<
        WebSocketConnectionImpl>(std::move(socket), std::move(peer_name), config, impl::frames::Masked::kNo);
}

std::shared_ptr<WebSocketConnection> MakeClientWebSocketConnection(
    std::unique_ptr<engine::io::RwBase>&& socket,
    engine::io::Sockaddr&& peer_name,
    const Config& config
) {
    return std::make_shared<
        WebSocketConnectionImpl>(std::move(socket), std::move(peer_name), config, impl::frames::Masked::kYes);
}

}  // namespace websocket

USERVER_NAMESPACE_END
