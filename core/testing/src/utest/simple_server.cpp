#include <userver/utest/simple_server.hpp>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/internal/net/net_listener.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

namespace {

class Client final {
 public:
  static void Run(engine::io::Socket&& socket, SimpleServer::OnRequest f);

 private:
  Client(engine::io::Socket&& socket, SimpleServer::OnRequest f)
      : socket_{std::move(socket)}, callback_{std::move(f)} {}

  [[nodiscard]] bool NeedsMoreReading() const {
    return (resp_.command == SimpleServer::Response::kTryReadMore);
  }

  [[nodiscard]] bool NeedsNewRequest() const {
    return (resp_.command == SimpleServer::Response::kWriteAndContinue);
  }

  void StartNewRequest() { incoming_data_.clear(); }

  std::size_t ReadSome();

  void WriteResponse();

  static constexpr std::size_t kReadBufferChunkSize = 1024 * 1024 * 1;

  engine::io::Socket socket_;
  SimpleServer::OnRequest callback_;

  SimpleServer::Request incoming_data_{};
  std::size_t previously_received_{0};
  SimpleServer::Response resp_{};
};

void Client::Run(engine::io::Socket&& socket, SimpleServer::OnRequest f) {
  LOG_TRACE() << "New client";
  Client c{std::move(socket), std::move(f)};
  do {
    const auto deadline =
        engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
    c.StartNewRequest();

    do {
      if (!c.ReadSome()) {
        return;
      }

      if (engine::current_task::IsCancelRequested()) {
        return;
      }

      if (deadline.IsReached()) {
        ADD_FAILURE() << "Shutting down slow request";
        return;
      }
    } while (c.NeedsMoreReading());

    c.WriteResponse();

  } while (c.NeedsNewRequest() && !engine::current_task::IsCancelRequested());
}

std::size_t Client::ReadSome() {
  previously_received_ = incoming_data_.size();
  incoming_data_.resize(previously_received_ + kReadBufferChunkSize);

  auto received =
      socket_.RecvSome(incoming_data_.data() + previously_received_,
                       incoming_data_.size() - previously_received_, {});

  if (!received) {
    LOG_TRACE() << "Remote peer shut down the connection";
    return 0;
  }

  incoming_data_.resize(previously_received_ + received);
  resp_ = callback_(incoming_data_);

  return received;
}

void Client::WriteResponse() {
  [[maybe_unused]] size_t sent =
      socket_.SendAll(resp_.data_to_send.data(), resp_.data_to_send.size(), {});
}

}  // namespace

class SimpleServer::Impl {
 public:
  Impl(OnRequest callback, Protocol protocol);

  [[nodiscard]] Port GetPort() const { return listener_.port; };
  [[nodiscard]] Protocol GetProtocol() const {
    switch (listener_.addr.Domain()) {
      case engine::io::AddrDomain::kInet:
        return Protocol::kTcpIpV4;
      case engine::io::AddrDomain::kInet6:
        return Protocol::kTcpIpV6;
      default:
        UINVARIANT(false, "Unexpected listener domain");
    }
  };

 private:
  OnRequest callback_;
  internal::net::TcpListener listener_;

  concurrent::BackgroundTaskStorage client_tasks_storage_;
  engine::Task listener_task_;

  void StartPortListening();
};

SimpleServer::Impl::Impl(OnRequest callback, Protocol protocol)
    : callback_{std::move(callback)},
      listener_{protocol == Protocol::kTcpIpV6
                    ? internal::net::IpVersion::kV6
                    : internal::net::IpVersion::kV4} {
  EXPECT_TRUE(callback_)
      << "SimpleServer must be started with a request callback";

  StartPortListening();
}

void SimpleServer::Impl::StartPortListening() {
  // NOLINTNEXTLINE(cppcoreguidelines-slicing)
  listener_task_ = engine::AsyncNoSpan([this, cb = callback_]() mutable {
    while (!engine::current_task::IsCancelRequested()) {
      auto socket = listener_.socket.Accept({});

      LOG_TRACE() << "SimpleServer accepted socket";

      client_tasks_storage_.AsyncDetach(
          "client", [cb = cb, s = std::move(socket)]() mutable {
            Client::Run(std::move(s), cb);
          });
    }
  });
}

SimpleServer::SimpleServer(OnRequest callback, Protocol protocol)
    : pimpl_{std::make_unique<Impl>(std::move(callback), protocol)} {}

SimpleServer::~SimpleServer() = default;

unsigned short SimpleServer::GetPort() const { return pimpl_->GetPort(); }

std::string SimpleServer::GetBaseUrl(Schema type) const {
  std::string url = type == Schema::kHttp ? "http://" : "https://";
  switch (pimpl_->GetProtocol()) {
    case kTcpIpV4:
      url += "127.0.0.1:";
      break;
    case kTcpIpV6:
      url += "[::1]:";
      break;
  }

  url += std::to_string(GetPort());

  return url;
}

}  // namespace utest

USERVER_NAMESPACE_END
