#include "simple_server.hpp"

#include <vector>

#include <engine/async.hpp>
#include <engine/io/socket.hpp>
#include <engine/task/cancel.hpp>

#include <gtest/gtest.h>

namespace testing {

namespace {

class Client {
 public:
  static void Run(engine::io::Socket socket, SimpleServer::OnRequest& f);

 private:
  Client(engine::io::Socket socket, SimpleServer::OnRequest& f)
      : socket_{std::move(socket)}, callback_{f} {}

  bool NeedsMoreReading() const {
    return (resp_.command == SimpleServer::Response::kTryReadMore);
  }

  bool NeedsNewRequest() const {
    return (resp_.command == SimpleServer::Response::kWriteAndContinue);
  }

  void StartNewRequest() { incomming_data_.clear(); }

  void ReadSome();

  void WriteResponse();

  static constexpr std::size_t kReadBufferChunkSize = 1024 * 1024 * 1;

  engine::io::Socket socket_;
  SimpleServer::OnRequest& callback_;

  SimpleServer::Request incomming_data_{};
  std::size_t previously_received_{0};
  SimpleServer::Response resp_{};
};

void Client::Run(engine::io::Socket socket, SimpleServer::OnRequest& f) {
  Client c{std::move(socket), f};
  do {
    c.StartNewRequest();
    do {
      c.ReadSome();

      if (engine::current_task::IsCancelRequested()) {
        return;
      }
    } while (c.NeedsMoreReading());

    c.WriteResponse();

  } while (c.NeedsNewRequest() && !engine::current_task::IsCancelRequested());
}

void Client::ReadSome() {
  previously_received_ = incomming_data_.size();
  incomming_data_.resize(previously_received_ + kReadBufferChunkSize);

  std::size_t received =
      socket_.RecvSome(&incomming_data_[0] + previously_received_,
                       incomming_data_.size() - previously_received_, {});

  incomming_data_.resize(previously_received_ + received);
  resp_ = callback_(incomming_data_);
}

void Client::WriteResponse() {
  socket_.SendAll(resp_.data_to_send.data(), resp_.data_to_send.size(), {});
}

}  // namespace

class SimpleServer::Impl {
 public:
  Impl(Ports ports, OnRequest f, Protocol protocol);

 private:
  OnRequest callback_;

  using acceptors_t = std::vector<engine::Task>;
  acceptors_t acceptors_;

  void Validate();
  void Accept(acceptors_t::iterator acceptor);

  static engine::io::Addr MakeLoopbackAddress(unsigned short port, Protocol p);
  void StartPortListening(unsigned short port, Protocol protocol);
};

SimpleServer::Impl::Impl(Ports ports, OnRequest f, Protocol protocol)
    : callback_{std::move(f)} {
  for (auto port : ports) {
    StartPortListening(port, protocol);
  }

  Validate();
}

void SimpleServer::Impl::Validate() {
  ASSERT_TRUE(!acceptors_.empty())
      << "SimpleServer must be started with at least one listen port";

  ASSERT_TRUE(callback_)
      << "SimpleServer must be started with a request callback";
}

engine::io::Addr SimpleServer::Impl::MakeLoopbackAddress(unsigned short port,
                                                         Protocol p) {
  engine::io::AddrStorage addr_storage;

  switch (p) {
    case kTcpIpV4: {
      auto* sa = addr_storage.As<struct sockaddr_in>();
      sa->sin_family = AF_INET;
      sa->sin_port = htons(port);
      sa->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
      return engine::io::Addr(addr_storage, SOCK_STREAM, 0);
    }
    case kTcpIpV6: {
      auto* sa = addr_storage.As<struct sockaddr_in6>();
      sa->sin6_family = AF_INET6;
      sa->sin6_port = htons(port);
      sa->sin6_addr = in6addr_loopback;
      return engine::io::Addr(addr_storage, SOCK_STREAM, 0);
    }
  }
}

void SimpleServer::Impl::StartPortListening(unsigned short port,
                                            Protocol protocol) {
  engine::io::Addr addr = MakeLoopbackAddress(port, protocol);

  // Starting acceptor in this coro to avoid errors when acceptor has not
  // started listing yet and someone is already connecting...
  auto acceptor = engine::io::Listen(addr);

  acceptors_.emplace_back(
      engine::Async([ this, acceptor = std::move(acceptor) ]() mutable {
        while (!engine::current_task::IsCancelRequested()) {
          auto socket = acceptor.Accept({});

          engine::Async([ this, s = std::move(socket) ]() mutable {
            Client::Run(std::move(s), this->callback_);
          })
              .Detach();
        }
      }));
}

SimpleServer::SimpleServer(Ports ports, OnRequest callback, Protocol p)
    : pimpl_{std::make_unique<Impl>(ports, std::move(callback), p)} {}

SimpleServer::~SimpleServer() = default;

}  // namespace testing
