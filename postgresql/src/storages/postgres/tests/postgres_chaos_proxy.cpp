#include <storages/postgres/tests/postgres_chaos_proxy.hpp>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/io/exception.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/logging/log.hpp>

#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <unordered_set>
#include <vector>

#include <arpa/inet.h>
#include <netinet/tcp.h>

USERVER_NAMESPACE_BEGIN

namespace {

struct PgMessage {
    char type;
    std::uint32_t length;
    std::vector<std::uint8_t> full_message;
    std::span<std::uint8_t> body;
};

void ForwardProxy(engine::io::Socket& src, engine::io::Socket& dst) {
    std::array<char, 1024> buf{};
    while (!engine::current_task::ShouldCancel()) {
        const auto bytes_read = src.RecvSome(buf.data(), buf.size(), {});
        if (!bytes_read) {
            throw std::runtime_error{"Failed to read data"};
        }

        const auto bytes_written = dst.SendAll(buf.data(), bytes_read, {});
        if (bytes_written != bytes_read) {
            throw std::runtime_error{"Failed to write data"};
        }
    }
}

bool RecvExact(engine::io::Socket& sock, std::uint8_t* buf, std::size_t size) {
    std::size_t total = 0;
    while (total < size) {
        const auto n = sock.RecvSome(buf + total, size - total, {});

        if (!n) {
            return false;
        }

        total += n;
    }
    return true;
}

PgMessage ReadPgMessage(engine::io::Socket& sock) {
    static constexpr auto kHeaderSize = 5;

    PgMessage message{};
    message.full_message.resize(kHeaderSize);

    if (!RecvExact(sock, message.full_message.data(), kHeaderSize)) {
        throw std::runtime_error{"Failed to read PG message header"};
    }

    message.type = message.full_message[0];
    std::memcpy(&message.length, message.full_message.data() + 1, sizeof(message.length));
    message.length = ntohl(message.length);

    if (message.length < 4) {
        throw std::runtime_error{"Invalid PG message length: " + std::to_string(message.length)};
    }

    const auto message_body_size = message.length - 4;

    message.full_message.resize(kHeaderSize + message_body_size);
    if (message_body_size > 0 && !RecvExact(sock, message.full_message.data() + kHeaderSize, message_body_size)) {
        throw std::runtime_error{"Failed to read PG message body"};
    }

    message.body = std::span{message.full_message.data() + kHeaderSize, message_body_size};

    return message;
}

void WritePgMessage(const PgMessage& message, engine::io::Socket& sock) {
    if (sock.SendAll(message.full_message.data(), message.full_message.size(), {}) != message.full_message.size()) {
        throw std::runtime_error{"Failed to write PG message"};
    }
}

void BackwardProxy(engine::io::Socket& src, engine::io::Socket& dst) {
    static const std::unordered_set<std::uint8_t> kResponseMessagesToDelay = {
        'Z'  // ReadyForQuery
    };

    // We need explicitly handle first message, which would be `N`
    // meaning, than GSSAPI is not required, since it does not
    // follow PG protocol message format
    std::uint8_t gssapi_response{};
    if (!RecvExact(src, &gssapi_response, 1)) {
        throw std::runtime_error{"Failed to read data"};
    }
    if (dst.SendAll(&gssapi_response, 1, {}) != 1) {
        throw std::runtime_error{"Failed to write data"};
    }

    while (!engine::current_task::ShouldCancel()) {
        const auto message = ReadPgMessage(src);

        // Intentionally delay some responses in order to trigger
        // incorrect handling of PG messages
        if (kResponseMessagesToDelay.contains(message.type)) {
            engine::InterruptibleSleepFor(std::chrono::milliseconds{1});
        }

        WritePgMessage(message, dst);
    }
}

}  // namespace

PostgresChaosProxy::PostgresChaosProxy(
    engine::TaskProcessor& task_processor,
    std::string postgres_host,
    std::uint16_t postgres_port
)
    : task_processor_(task_processor),
      task_storage_()
{
    if (postgres_host == "localhost") {
        postgres_addr_ = engine::io::Sockaddr::MakeIPv4LoopbackAddress();
    } else {
        postgres_addr_ = engine::io::Sockaddr::MakeIPSocketAddress(postgres_host);
    }
    postgres_addr_.SetPort(postgres_port);

    auto proxy_addr = engine::io::Sockaddr::MakeIPv4LoopbackAddress();
    proxy_addr.SetPort(0);

    proxy_socket_ = engine::io::Socket{engine::io::AddrDomain::kInet, engine::io::SocketType::kTcp};

    proxy_socket_.SetOption(SOL_SOCKET, SO_REUSEADDR, 1);
    proxy_socket_.Bind(proxy_addr);

    proxy_socket_.Listen();

    // NOLINTNEXTLINE (cppcoreguidelines-prefer-member-initializer)
    port_ = proxy_socket_.Getsockname().Port();

    acceptor_task_ = engine::AsyncNoTracing(task_processor_, &PostgresChaosProxy::AcceptorLoop, this);
}

PostgresChaosProxy::~PostgresChaosProxy() {
    acceptor_task_.SyncCancel();
    proxy_socket_.Close();

    task_storage_.CancelAndWait();
}

std::uint16_t PostgresChaosProxy::GetPort() const { return port_; }

void PostgresChaosProxy::AcceptorLoop() {
    while (!engine::current_task::ShouldCancel()) {
        engine::io::Socket proxied_socket = proxy_socket_.Accept({});
        proxied_socket.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);

        engine::io::Socket db_socket{postgres_addr_.Domain(), engine::io::SocketType::kTcp};
        db_socket.Connect(postgres_addr_, {});

        task_storage_.Detach(engine::AsyncNoTracing(
            task_processor_,
            &PostgresChaosProxy::ProcessSocket,
            this,
            std::move(proxied_socket),
            std::move(db_socket)
        ));
    }
}

void PostgresChaosProxy::ProcessSocket(engine::io::Socket proxied_socket, engine::io::Socket db_socket) {
    auto forward_task =
        engine::AsyncNoTracing(task_processor_, &ForwardProxy, std::ref(proxied_socket), std::ref(db_socket));
    auto backward_task =
        engine::AsyncNoTracing(task_processor_, &BackwardProxy, std::ref(db_socket), std::ref(proxied_socket));

    engine::WaitAny(forward_task, backward_task);

    if (forward_task.IsFinished()) {
        backward_task.RequestCancel();
    } else if (backward_task.IsFinished()) {
        forward_task.RequestCancel();
    }

    forward_task.Wait();
    backward_task.Wait();

    proxied_socket.Close();
    db_socket.Close();
}

USERVER_NAMESPACE_END
