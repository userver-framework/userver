#include "native_client_factory.hpp"

#include <netinet/in.h>
#include <netinet/tcp.h>

#include <clickhouse/base/input.h>
#include <clickhouse/base/output.h>
#include <clickhouse/base/socket.h>
#include <clickhouse/client.h>

#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/io/tls_wrapper.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/assert.hpp>

#include <storages/clickhouse/impl/tracing_tags.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::impl {

namespace {

using Socket = engine::io::Socket;
using TlsSocket = engine::io::TlsWrapper;

using ConnectionMode = ConnectionSettings::ConnectionMode;
using CompressionMethod = ConnectionSettings::CompressionMethod;

constexpr std::chrono::milliseconds kConnectTimeout{2000};

template <typename T>
class ClickhouseSocketInput final : public clickhouse_cpp::InputStream {
 public:
  ClickhouseSocketInput(T& socket, engine::Deadline& deadline)
      : socket_{socket}, deadline_{deadline} {}
  ~ClickhouseSocketInput() override = default;

 protected:
  bool Skip([[maybe_unused]] size_t unused) override { return false; }

  size_t DoRead(void* buf, size_t len) override {
    if (deadline_.IsReached()) throw engine::io::IoTimeout{};

    const auto read = socket_.RecvSome(buf, len, deadline_);

    if (!read) throw engine::io::IoException{"socket reset by peer"};

    return read;
  }

 private:
  T& socket_;
  engine::Deadline& deadline_;
};

template <typename T>
class ClickhouseSocketOutput final : public clickhouse_cpp::OutputStream {
 public:
  ClickhouseSocketOutput(T& socket, engine::Deadline& deadline)
      : socket_{socket}, deadline_{deadline} {}
  ~ClickhouseSocketOutput() override = default;

 protected:
  size_t DoWrite(const void* data, size_t len) override {
    if (deadline_.IsReached()) throw engine::io::IoTimeout{};

    const auto sent = socket_.SendAll(data, len, deadline_);
    if (sent != len) throw engine::io::IoException{"broken pipe?"};

    return sent;
  }

 private:
  T& socket_;
  engine::Deadline& deadline_;
};

Socket CreateSocket(engine::io::Sockaddr addr, engine::Deadline deadline) {
  Socket socket{addr.Domain(), engine::io::SocketType::kTcp};
  socket.SetOption(IPPROTO_TCP, TCP_NODELAY, 1);
  socket.Connect(addr, deadline);

  return socket;
}

class ClickhouseSocketAdapter : public clickhouse_cpp::SocketBase {
 public:
  ClickhouseSocketAdapter(engine::io::Sockaddr addr, engine::Deadline& deadline)
      : deadline_{deadline}, socket_{CreateSocket(addr, deadline_)} {}

  ~ClickhouseSocketAdapter() override { socket_.Close(); }

  std::unique_ptr<clickhouse_cpp::InputStream> makeInputStream()
      const override {
    return std::make_unique<ClickhouseSocketInput<Socket>>(socket_, deadline_);
  }

  std::unique_ptr<clickhouse_cpp::OutputStream> makeOutputStream()
      const override {
    return std::make_unique<ClickhouseSocketOutput<Socket>>(socket_, deadline_);
  }

 private:
  engine::Deadline& deadline_;
  mutable Socket socket_;
};

class ClickhouseTlsSocketAdapter : public clickhouse_cpp::SocketBase {
 public:
  ClickhouseTlsSocketAdapter(engine::io::Sockaddr addr,
                             engine::Deadline& deadline)
      : deadline_{deadline},
        tls_socket_{engine::io::TlsWrapper::StartTlsClient(
            CreateSocket(addr, deadline_), {}, deadline_)} {}

  std::unique_ptr<clickhouse_cpp::InputStream> makeInputStream()
      const override {
    return std::make_unique<ClickhouseSocketInput<TlsSocket>>(tls_socket_,
                                                              deadline_);
  }

  std::unique_ptr<clickhouse_cpp::OutputStream> makeOutputStream()
      const override {
    return std::make_unique<ClickhouseSocketOutput<TlsSocket>>(tls_socket_,
                                                               deadline_);
  }

 private:
  engine::Deadline& deadline_;
  mutable TlsSocket tls_socket_;
};

class ClickhouseSocketFactory final : public clickhouse_cpp::SocketFactory {
 public:
  ClickhouseSocketFactory(clients::dns::Resolver& resolver, ConnectionMode mode,
                          engine::Deadline& operations_deadline)
      : resolver_{resolver},
        mode_{mode},
        operations_deadline_{operations_deadline} {}

  ~ClickhouseSocketFactory() override = default;

  std::unique_ptr<clickhouse_cpp::SocketBase> connect(
      const clickhouse_cpp::ClientOptions& opts) override {
    auto addrs = resolver_.Resolve(opts.host, operations_deadline_);

    for (auto&& current_addr : addrs) {
      current_addr.SetPort(static_cast<int>(opts.port));

      try {
        switch (mode_) {
          case ConnectionMode::kNonSecure:
            return std::make_unique<ClickhouseSocketAdapter>(
                current_addr, operations_deadline_);
          case ConnectionMode::kSecure:
            return std::make_unique<ClickhouseTlsSocketAdapter>(
                current_addr, operations_deadline_);
        }
      } catch (const std::exception&) {
      }
    }

    throw std::runtime_error{
        "Could not connect to any of the resolved addresses"};
  }

  void sleepFor(const std::chrono::milliseconds& duration) override {
    engine::SleepFor(duration);
  }

 private:
  clients::dns::Resolver& resolver_;
  ConnectionMode mode_;

  engine::Deadline& operations_deadline_;
};

clickhouse_cpp::CompressionMethod GetCompressionMethod(
    CompressionMethod method) {
  switch (method) {
    case CompressionMethod::kNone:
      return clickhouse_cpp::CompressionMethod::None;
    case CompressionMethod::kLZ4:
      return clickhouse_cpp::CompressionMethod::LZ4;
  }
}

}  // namespace

NativeClientWrapper::NativeClientWrapper(
    clients::dns::Resolver& resolver,
    const clickhouse_cpp::ClientOptions& options, ConnectionMode mode) {
  SetDeadline(engine::Deadline::FromDuration(kConnectTimeout));

  auto socket_factory = std::make_unique<ClickhouseSocketFactory>(
      resolver, mode, operations_deadline_);
  native_client_ = std::make_unique<clickhouse_cpp::Client>(
      options, std::move(socket_factory));
}

NativeClientWrapper::~NativeClientWrapper() = default;

void NativeClientWrapper::Execute(const clickhouse_cpp::Query& query,
                                  engine::Deadline deadline) {
  SetDeadline(deadline);
  native_client_->Execute(query);
}

void NativeClientWrapper::Insert(const std::string& table_name,
                                 const clickhouse_cpp::Block& block,
                                 engine::Deadline deadline) {
  SetDeadline(deadline);
  native_client_->Insert(table_name, block);
}

void NativeClientWrapper::Ping(engine::Deadline deadline) {
  SetDeadline(deadline);
  native_client_->Ping();
}

void NativeClientWrapper::SetDeadline(engine::Deadline deadline) {
  operations_deadline_ = deadline;
}

NativeClientWrapper NativeClientFactory::Create(
    clients::dns::Resolver& resolver, const EndpointSettings& endpoint,
    const AuthSettings& auth, const ConnectionSettings& connection_settings) {
  const auto options = clickhouse_cpp::ClientOptions{}
                           .SetHost(endpoint.host)
                           .SetPort(endpoint.port)
                           .SetUser(auth.user)
                           .SetPassword(auth.password)
                           .SetDefaultDatabase(auth.database)
                           .SetCompressionMethod(GetCompressionMethod(
                               connection_settings.compression_method));

  tracing::Span span{scopes::kConnect};
  return NativeClientWrapper{resolver, options,
                             connection_settings.connection_mode};
}

}  // namespace storages::clickhouse::impl

USERVER_NAMESPACE_END
