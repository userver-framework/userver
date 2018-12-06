#pragma once

#include <functional>
#include <initializer_list>
#include <memory>
#include <string>

namespace testing {

/// Toy server for simple network testing.
///
/// In constructor opens specified ports in localhost address and listens on
/// them. On each accepted data packet calls user callback.
///
/// Example usage:
///
///  SimpleServer::Response assert_received_ok(const SimpleServer::Request& r) {
///    EXPECT_EQ(r, "OK") << "SimpleServer received: " << r;
///    return {"RESPONSE DATA", SimpleServer::Response::kWriteAndClose};
///  }
///
///  TEST(Something, SendOk) {
///    TestInCoro([] {
///      SimpleServer s({8080, 8042}, assert_received_ok);
///      // ... invoke code that sends "OK" to localhost:8080 or localhost:8042.
///    });
///  }

class SimpleServer {
 public:
  struct Response {
    enum Commands {
      kWriteAndClose,
      kTryReadMore,
      kWriteAndContinue,
    };

    std::string data_to_send{};
    Commands command{kWriteAndClose};
  };

  using Request = std::string;
  using OnRequest = std::function<Response(const Request&)>;

  using Ports = std::initializer_list<unsigned short>;
  enum Protocol { kTcpIpV4, kTcpIpV6 };

  SimpleServer(Ports ports, OnRequest callback, Protocol protocol = kTcpIpV4);
  ~SimpleServer();

 private:
  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

}  // namespace testing
