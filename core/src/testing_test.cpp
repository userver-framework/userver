#include <userver/utest/utest.hpp>

// Test that pfr is includable
#include <boost/pfr/core.hpp>

#include <userver/utest/simple_server.hpp>

#include <userver/engine/io/socket.hpp>

USERVER_NAMESPACE_BEGIN

/// [Sample SimpleServer usage]
namespace {
using utest::SimpleServer;

const std::string kOkRequest = "OK";
const std::string kOkResponse = "OK RESPONSE DATA";

SimpleServer::Response assert_received_ok(const SimpleServer::Request& r) {
  EXPECT_EQ(r, kOkRequest) << "SimpleServer received: " << r;
  return {kOkResponse, SimpleServer::Response::kWriteAndClose};
}

}  // namespace

UTEST(SimpleServer, ExampleTcpIpV4) {
  SimpleServer s(assert_received_ok);

  // ... invoke code that sends "OK" to localhost
  engine::io::Sockaddr addr;
  auto* sa = addr.As<struct sockaddr_in>();
  sa->sin_family = AF_INET;
  // NOLINTNEXTLINE(hicpp-no-assembler,readability-isolate-declaration)
  sa->sin_port = htons(s.GetPort());
  // NOLINTNEXTLINE(hicpp-no-assembler,readability-isolate-declaration)
  sa->sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  engine::io::Socket worksock{addr.Domain(), engine::io::SocketType::kStream};
  worksock.Connect(addr,
                   engine::Deadline::FromDuration(utest::kMaxTestWaitTime));
  ASSERT_EQ(kOkRequest.size(),
            worksock.SendAll(kOkRequest.data(), kOkRequest.size(), {}));

  std::string response;
  response.resize(100);
  const auto size = worksock.RecvAll(response.data(), response.size(), {});
  response.resize(size);
  EXPECT_EQ(response, kOkResponse) << "Received " << response;
}
/// [Sample SimpleServer usage]

UTEST(SimpleServer, NothingReceived) {
  const auto assert_received_nothing = [](const SimpleServer::Request& r) {
    EXPECT_TRUE(false) << "SimpleServer received: " << r;
    return SimpleServer::Response{"", SimpleServer::Response::kWriteAndClose};
  };
  SimpleServer{assert_received_nothing};
}

UTEST(SimpleServer, ExampleTcpIpV6) {
  SimpleServer s(assert_received_ok, SimpleServer::kTcpIpV6);

  // ... invoke code that sends "OK" to localhost:8080 or localhost:8042.
  engine::io::Sockaddr addr;
  auto* sa = addr.As<struct sockaddr_in6>();
  sa->sin6_family = AF_INET6;
  // NOLINTNEXTLINE(hicpp-no-assembler,readability-isolate-declaration)
  sa->sin6_port = htons(s.GetPort());
  sa->sin6_addr = in6addr_loopback;

  engine::io::Socket worksock{addr.Domain(), engine::io::SocketType::kStream};
  worksock.Connect(addr,
                   engine::Deadline::FromDuration(utest::kMaxTestWaitTime));
  ASSERT_EQ(kOkRequest.size(),
            worksock.SendAll(kOkRequest.data(), kOkRequest.size(), {}));

  std::string response;
  response.resize(100);
  const auto size = worksock.RecvAll(response.data(), response.size(), {});
  response.resize(size);
  EXPECT_EQ(response, kOkResponse) << "Received " << response;
}

UTEST(SimpleServer, ExampleTcpIpV4Twice) {
  auto assert_received_twice = [i = 0](const SimpleServer::Request& r) mutable {
    EXPECT_EQ(r, kOkRequest) << "SimpleServer received: " << r;
    EXPECT_LE(++i, 2) << "Callback was called more than twice: " << i;

    const auto command = (i == 1 ? SimpleServer::Response::kWriteAndContinue
                                 : SimpleServer::Response::kWriteAndClose);

    return SimpleServer::Response{kOkResponse, command};
  };

  SimpleServer s(assert_received_twice);

  // ... invoke code that sends "OK" to localhost:8080 or localhost:8042.
  engine::io::Sockaddr addr;
  auto* sa = addr.As<struct sockaddr_in>();
  sa->sin_family = AF_INET;
  // NOLINTNEXTLINE(hicpp-no-assembler,readability-isolate-declaration)
  sa->sin_port = htons(s.GetPort());
  // NOLINTNEXTLINE(readability-isolate-declaration,hicpp-no-assembler)
  sa->sin_addr.s_addr = htonl(INADDR_LOOPBACK);

  engine::io::Socket worksock{addr.Domain(), engine::io::SocketType::kStream};
  worksock.Connect(addr,
                   engine::Deadline::FromDuration(utest::kMaxTestWaitTime));

  ASSERT_EQ(kOkRequest.size(),
            worksock.SendAll(kOkRequest.data(), kOkRequest.size(), {}));
  std::string response;
  response.resize(100);
  const auto size = worksock.RecvSome(response.data(), response.size(), {});
  response.resize(size);
  EXPECT_EQ(response, kOkResponse) << "Received " << response;

  ASSERT_EQ(kOkRequest.size(),
            worksock.SendAll(kOkRequest.data(), kOkRequest.size(), {}));
  response.clear();
  response.resize(100);
  const auto size2 = worksock.RecvAll(response.data(), response.size(), {});
  response.resize(size2);
  EXPECT_EQ(response, kOkResponse) << "Received " << response;

  EXPECT_EQ(0, worksock.RecvAll(response.data(), response.size(), {}));
}

USERVER_NAMESPACE_END
