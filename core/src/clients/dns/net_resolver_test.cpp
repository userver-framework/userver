#include <netinet/in.h>
#include <sys/socket.h>

#include <clients/dns/net_resolver.hpp>
#include <userver/clients/dns/exception.hpp>
#include <userver/engine/io/sockaddr.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/utest/dns_server_mock.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/mock_now.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using Mock = utest::DnsServerMock;

const auto kV4Sockaddr1 = [] {
  engine::io::Sockaddr sockaddr;
  auto* sa = sockaddr.As<sockaddr_in>();
  sa->sin_family = AF_INET;
  // NOLINTNEXTLINE(hicpp-no-assembler,readability-isolate-declaration)
  sa->sin_addr.s_addr = htonl(0x4D583737);
  return sockaddr;
}();
constexpr auto kV4String1{"77.88.55.55"};

const auto kV4Sockaddr2 = [] {
  engine::io::Sockaddr sockaddr;
  auto* sa = sockaddr.As<sockaddr_in>();
  sa->sin_family = AF_INET;
  // NOLINTNEXTLINE(hicpp-no-assembler,readability-isolate-declaration)
  sa->sin_addr.s_addr = htonl(0x4D58373C);
  return sockaddr;
}();
constexpr auto kV4String2{"77.88.55.60"};

const auto kV6Sockaddr = [] {
  engine::io::Sockaddr sockaddr;
  auto* sa = sockaddr.As<sockaddr_in6>();
  sa->sin6_family = AF_INET6;
  sa->sin6_addr.s6_addr[0] = 0x2A;
  sa->sin6_addr.s6_addr[1] = 0x02;
  sa->sin6_addr.s6_addr[2] = 0x06;
  sa->sin6_addr.s6_addr[3] = 0xB8;
  sa->sin6_addr.s6_addr[5] = 0x0A;
  sa->sin6_addr.s6_addr[15] = 0x0A;
  return sockaddr;
}();
constexpr auto kV6String{"2a02:6b8:a::a"};

auto GetResolver(Mock& mock) {
  // Resolver is big for stack
  return std::make_unique<clients::dns::NetResolver>(
      engine::current_task::GetTaskProcessor(), utest::kMaxTestWaitTime, 1,
      std::vector<std::string>{mock.GetServerAddress()});
}

auto Open1025DummySockets() {
  std::vector<engine::io::Socket> dummy_sockets;
  dummy_sockets.reserve(1025);
  for (int i = 0; i < 1025; i++) {
    dummy_sockets.emplace_back(engine::io::AddrDomain::kInet,
                               engine::io::SocketType::kTcp);
  }

  return dummy_sockets;
}

::testing::AssertionResult IsExpectedV4Address(
    const char* text, const engine::io::Sockaddr& addr) {
  const auto addr_str = addr.PrimaryAddressString();
  if (addr_str == kV4String1 || addr_str == kV4String2) {
    return ::testing::AssertionSuccess();
  }
  return ::testing::AssertionFailure()
         << text << " is not an expected IPv4 address";
}

}  // namespace

UTEST(NetResolver, Smoke) {
  Mock mock{[](const Mock::DnsQuery& query) -> Mock::DnsAnswerVector {
    if (query.type == Mock::RecordType::kA &&
        (query.name == "yandex.ru" || query.name == "v4.yandex.ru")) {
      return {{query.type, kV4Sockaddr1, 13}, {query.type, kV4Sockaddr2, 42}};
    } else if (query.type == Mock::RecordType::kAAAA &&
               (query.name == "yandex.ru" || query.name == "v6.yandex.ru")) {
      return {{query.type, kV6Sockaddr, 1337}};
    } else if ((query.type == Mock::RecordType::kAAAA &&
                query.name == "v4.yandex.ru") ||
               (query.type == Mock::RecordType::kA &&
                query.name == "v6.yandex.ru")) {
      return {};
    }
    throw std::exception{};
  }};

  auto resolver = GetResolver(mock);

  auto resolve_start = utils::datetime::MockNow();
  auto result = resolver->Resolve("yandex.ru").get();
  ASSERT_EQ(result.addrs.size(), 3);
  EXPECT_EQ(result.addrs[0].PrimaryAddressString(), kV6String);
  EXPECT_PRED_FORMAT1(IsExpectedV4Address, result.addrs[1]);
  EXPECT_PRED_FORMAT1(IsExpectedV4Address, result.addrs[2]);
  EXPECT_LE(result.received_at - resolve_start, utest::kMaxTestWaitTime);
  EXPECT_EQ(result.ttl, std::chrono::seconds{13});

  resolve_start = utils::datetime::MockNow();
  result = resolver->Resolve("v4.yandex.ru").get();
  ASSERT_EQ(result.addrs.size(), 2);
  EXPECT_PRED_FORMAT1(IsExpectedV4Address, result.addrs[0]);
  EXPECT_PRED_FORMAT1(IsExpectedV4Address, result.addrs[1]);
  EXPECT_LE(result.received_at - resolve_start, utest::kMaxTestWaitTime);
  EXPECT_EQ(result.ttl, std::chrono::seconds{13});

  resolve_start = utils::datetime::MockNow();
  result = resolver->Resolve("v6.yandex.ru").get();
  ASSERT_EQ(result.addrs.size(), 1);
  EXPECT_EQ(result.addrs[0].PrimaryAddressString(), kV6String);
  EXPECT_LE(result.received_at - resolve_start, utest::kMaxTestWaitTime);
  EXPECT_EQ(result.ttl, std::chrono::seconds{1337});
}

UTEST(NetResolver, EmptyResponse) {
  Mock mock{[](const auto&) { return Mock::DnsAnswerVector{}; }};

  auto resolver = GetResolver(mock);

  auto future = resolver->Resolve("test");
  UEXPECT_THROW(future.get(), clients::dns::NotResolvedException);
}

UTEST(NetResolver, NetworkTimeout) {
  Mock mock(
      [](const auto&) -> Mock::DnsAnswerVector { throw Mock::NoAnswer{}; });

  auto resolver =
      clients::dns::NetResolver{engine::current_task::GetTaskProcessor(),
                                std::chrono::milliseconds{100},
                                1,
                                {mock.GetServerAddress()}};

  auto future = resolver.Resolve("test");
  ASSERT_EQ(future.wait_for(utest::kMaxTestWaitTime),
            engine::FutureStatus::kReady);
  UEXPECT_THROW(future.get(), clients::dns::NotResolvedException);
}

UTEST(NetResolver, Cname) {
  Mock mock{[](const Mock::DnsQuery& query) -> Mock::DnsAnswerVector {
    if (query.name == "test") {
      if (query.type == Mock::RecordType::kA) {
        return {{Mock::RecordType::kCname, "yandex.ru", 0},
                {query.type, kV4Sockaddr1, 9},
                {query.type, kV4Sockaddr2, 8}};
      } else if (query.type == Mock::RecordType::kAAAA) {
        return {{Mock::RecordType::kCname, "yandex.ru", 0},
                {query.type, kV6Sockaddr, 7}};
      }
    } else if (query.name == "yandex.ru") {
      if (query.type == Mock::RecordType::kA) {
        return {{query.type, kV4Sockaddr1, 9}, {query.type, kV4Sockaddr2, 8}};
      } else if (query.type == Mock::RecordType::kAAAA) {
        return {{query.type, kV6Sockaddr, 7}};
      }
    }
    throw std::exception{};
  }};

  auto resolver = GetResolver(mock);

  const auto resolve_start = utils::datetime::MockNow();
  auto result = resolver->Resolve("test").get();
  ASSERT_EQ(result.addrs.size(), 3);
  EXPECT_EQ(result.addrs[0].PrimaryAddressString(), kV6String);
  EXPECT_PRED_FORMAT1(IsExpectedV4Address, result.addrs[1]);
  EXPECT_PRED_FORMAT1(IsExpectedV4Address, result.addrs[2]);
  EXPECT_LE(result.received_at - resolve_start, utest::kMaxTestWaitTime);
  EXPECT_EQ(result.ttl, std::chrono::seconds{7});
}

UTEST(NetResolver, ServerFailure) {
  Mock mock{
      [](const auto&) -> Mock::DnsAnswerVector { throw std::exception{}; }};

  auto resolver = GetResolver(mock);

  auto future = resolver->Resolve("test");
  UEXPECT_THROW(future.get(), clients::dns::NotResolvedException);
}

UTEST(NetResolver, PartialServerFailure) {
  size_t servfails_sent = 0;
  utest::DnsServerMock mock{[&](const Mock::DnsQuery& query) {
    if (query.name == "test" && query.type == Mock::RecordType::kAAAA) {
      return Mock::DnsAnswerVector{{query.type, kV6Sockaddr, 300}};
    }
    ++servfails_sent;
    throw std::exception{};
  }};

  auto resolver = GetResolver(mock);

  const auto resolve_start = utils::datetime::MockNow();
  auto result = resolver->Resolve("test").get();
  EXPECT_GE(servfails_sent, 1);
  ASSERT_EQ(result.addrs.size(), 1);
  EXPECT_EQ(result.addrs[0].PrimaryAddressString(), kV6String);
  EXPECT_LE(result.received_at - resolve_start, utest::kMaxTestWaitTime);
  EXPECT_EQ(result.ttl, std::chrono::seconds{300});
}

UTEST(NetResolver, Select1024) {
  Mock mock{[](const auto& query) {
    if (query.name == "test" && query.type == Mock::RecordType::kAAAA) {
      return Mock::DnsAnswerVector{{query.type, kV6Sockaddr, 300}};
    }
    return Mock::DnsAnswerVector{};
  }};

  /* select() needs fd_set, but it is limited to 1024 fd number */
  std::vector<engine::io::Socket> dummy_sockets;
  UEXPECT_NO_THROW(dummy_sockets = Open1025DummySockets())
      << "Looks like a limit on open file descriptors on your OS is too low. "
         "Increase the limit by running `ulimit -n 4096` on POSIX systems.";

  auto resolver = GetResolver(mock);

  const auto resolve_start = utils::datetime::MockNow();
  auto result = resolver->Resolve("test").get();
  EXPECT_EQ(result.addrs.size(), 1);
  EXPECT_EQ(result.addrs[0].PrimaryAddressString(), kV6String);
  EXPECT_LE(result.received_at - resolve_start, utest::kMaxTestWaitTime);
  EXPECT_EQ(result.ttl, std::chrono::seconds{300});
}

USERVER_NAMESPACE_END
