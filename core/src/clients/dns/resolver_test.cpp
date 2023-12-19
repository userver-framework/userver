#include <string_view>
#include <vector>

#include <fmt/format.h>

#include <userver/clients/dns/config.hpp>
#include <userver/clients/dns/exception.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/dns_server_mock.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/mock_now.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr auto kTestHosts = R"(
127.0.0.1 mycomputer
::1 mycomputer
127.0.0.3 disappearing
)";

constexpr auto kReplacementHosts = R"(
127.0.0.2 localhost mycomputer
127.0.0.4 invalid fail
127.0.0.5 override
)";

const auto kNetV4Sockaddr = [] {
  engine::io::Sockaddr sockaddr;
  auto* sa = sockaddr.As<sockaddr_in>();
  sa->sin_family = AF_INET;
  // NOLINTNEXTLINE(hicpp-no-assembler,readability-isolate-declaration)
  sa->sin_addr.s_addr = htonl(0x4D583737);
  return sockaddr;
}();
constexpr auto kNetV4String{"77.88.55.55"};

const auto kNetV6Sockaddr = [] {
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
constexpr auto kNetV6String{"2a02:6b8:a::a"};

struct MockedResolver {
  using ServerMock = utest::DnsServerMock;

  MockedResolver(size_t cache_max_ttl, size_t cache_size_per_way)
      : hosts_file{[] {
          auto file = fs::blocking::TempFile::Create();
          fs::blocking::RewriteFileContents(file.GetPath(), kTestHosts);
          return file;
        }()},
        server_mock{[this](const ServerMock::DnsQuery& query)
                        -> ServerMock::DnsAnswerVector {
          engine::InterruptibleSleepFor(reply_delay);
          if (query.name == "fail") throw std::exception{};

          if (query.type == ServerMock::RecordType::kA) {
            return {{query.type, kNetV4Sockaddr, 99999}};
          } else if (query.type == ServerMock::RecordType::kAAAA) {
            return {{query.type, kNetV6Sockaddr, 99999}};
          }
          throw std::exception{};
        }},
        resolver{
            engine::current_task::GetTaskProcessor(), [=] {
              clients::dns::ResolverConfig config;
              config.file_path = hosts_file.GetPath();
              config.file_update_interval = utest::kMaxTestWaitTime;
              config.network_timeout = utest::kMaxTestWaitTime;
              config.network_attempts = 1;
              config.cache_max_reply_ttl = std::chrono::seconds{cache_max_ttl};
              config.cache_failure_ttl = std::chrono::seconds{cache_max_ttl},
              config.cache_ways = 1;
              config.cache_size_per_way = cache_size_per_way;
              config.network_custom_servers = {server_mock.GetServerAddress()};
              return config;
            }()} {}

  clients::dns::Resolver* operator->() { return &resolver; }

  void ReplaceHosts() const {
    fs::blocking::RewriteFileContents(hosts_file.GetPath(), kReplacementHosts);
  }

  std::chrono::milliseconds reply_delay{0};
  fs::blocking::TempFile hosts_file;
  ServerMock server_mock;
  clients::dns::Resolver resolver;
};

using Expected = std::vector<std::string_view>;

::testing::AssertionResult CheckAddrs(const char* addrs_text,
                                      const char* /* expected_text */,
                                      const clients::dns::AddrVector& addrs,
                                      const Expected& expected) {
  static constexpr std::string_view kSeparator = ", ";

  const auto expected_str = fmt::to_string(fmt::join(expected, kSeparator));

  // will be better with views::transform and join
  fmt::memory_buffer buf;
  for (const auto& addr : addrs) {
    if (buf.size() > 0) buf.append(kSeparator);
    buf.append(addr.PrimaryAddressString());
  }
  const std::string_view got_str{buf.data(), buf.size()};

  if (got_str != expected_str) {
    return ::testing::AssertionFailure()
           << addrs_text << " returned wrong address list: expected ["
           << expected_str << "], got [" << got_str << ']';
  }
  return ::testing::AssertionSuccess();
}

}  // namespace

UTEST(Resolver, Smoke) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  MockedResolver resolver{1000, 1};

  EXPECT_PRED_FORMAT2(CheckAddrs,
                      resolver->Resolve("mycomputer", test_deadline),
                      (Expected{"::1", "127.0.0.1"}));

  EXPECT_PRED_FORMAT2(CheckAddrs,
                      resolver->Resolve("not-mycomputer", test_deadline),
                      (Expected{kNetV6String, kNetV4String}));

  UEXPECT_THROW(resolver->Resolve("fail", test_deadline),
                clients::dns::NotResolvedException);

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("127.0.0.1", test_deadline),
                      (Expected{"127.0.0.1"}));
  UEXPECT_THROW(resolver->Resolve("127.0.0.1:80", test_deadline),
                clients::dns::NotResolvedException);

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("::1", test_deadline),
                      (Expected{"::1"}));
  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("::1:80", test_deadline),
                      (Expected{"::0.1.0.128"}));

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("[::1]", test_deadline),
                      (Expected{"::1"}));
  UEXPECT_THROW(resolver->Resolve("[::1]:80", test_deadline),
                clients::dns::NotResolvedException);

  EXPECT_PRED_FORMAT2(CheckAddrs,
                      resolver->Resolve("[::ffff:127.0.0.1]", test_deadline),
                      (Expected{"::ffff:127.0.0.1"}));

  UEXPECT_THROW(resolver->Resolve("[not-mycomputer]", test_deadline),
                clients::dns::NotResolvedException);

  UEXPECT_THROW(resolver->Resolve("[mycomputer]", test_deadline),
                clients::dns::NotResolvedException);

  UEXPECT_THROW(resolver->Resolve("*.*", test_deadline),
                clients::dns::NotResolvedException);

  const auto& counters = resolver->GetLookupSourceCounters();
  EXPECT_EQ(counters.file, 1);
  EXPECT_EQ(counters.cached, 0);
  EXPECT_EQ(counters.cached_stale, 0);
  EXPECT_EQ(counters.cached_failure, 0);
  EXPECT_EQ(counters.network, 1);
  EXPECT_EQ(counters.network_failure, 1);
}

// 'localhost' should always return loopback IP -- RFC6761 6.3
UTEST(Resolver, Localhost) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  MockedResolver resolver{1000, 1};

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("localhost", test_deadline),
                      (Expected{"::1", "127.0.0.1"}));
  EXPECT_PRED_FORMAT2(CheckAddrs,
                      resolver->Resolve("subdomain.localhost", test_deadline),
                      (Expected{"::1", "127.0.0.1"}));

  resolver.ReplaceHosts();

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("localhost", test_deadline),
                      (Expected{"::1", "127.0.0.1"}));
  EXPECT_PRED_FORMAT2(CheckAddrs,
                      resolver->Resolve("subdomain.localhost", test_deadline),
                      (Expected{"::1", "127.0.0.1"}));

  resolver->ReloadHosts();

  // override in hosts does not work
  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("localhost", test_deadline),
                      (Expected{"::1", "127.0.0.1"}));
  EXPECT_PRED_FORMAT2(CheckAddrs,
                      resolver->Resolve("subdomain.localhost", test_deadline),
                      (Expected{"::1", "127.0.0.1"}));

  // FQDN works
  EXPECT_PRED_FORMAT2(CheckAddrs,
                      resolver->Resolve("localhost.", test_deadline),
                      (Expected{"::1", "127.0.0.1"}));
  EXPECT_PRED_FORMAT2(CheckAddrs,
                      resolver->Resolve("subdomain.localhost.", test_deadline),
                      (Expected{"::1", "127.0.0.1"}));

  // we correctly identify domains
  EXPECT_PRED_FORMAT2(CheckAddrs,
                      resolver->Resolve("not-localhost", test_deadline),
                      (Expected{kNetV6String, kNetV4String}));

  const auto& counters = resolver->GetLookupSourceCounters();
  EXPECT_EQ(counters.file, 0);
  EXPECT_EQ(counters.cached, 0);
  EXPECT_EQ(counters.cached_stale, 0);
  EXPECT_EQ(counters.cached_failure, 0);
  EXPECT_EQ(counters.network, 1);
  EXPECT_EQ(counters.network_failure, 0);
}

// 'invalid' should always fail -- RFC6761 6.4
UTEST(Resolver, Invalid) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  MockedResolver resolver{1000, 1};

  UEXPECT_THROW(resolver->Resolve("invalid", test_deadline),
                clients::dns::NotResolvedException);
  UEXPECT_THROW(resolver->Resolve("subdomain.invalid", test_deadline),
                clients::dns::NotResolvedException);

  resolver.ReplaceHosts();

  UEXPECT_THROW(resolver->Resolve("invalid", test_deadline),
                clients::dns::NotResolvedException);
  UEXPECT_THROW(resolver->Resolve("subdomain.invalid", test_deadline),
                clients::dns::NotResolvedException);

  resolver->ReloadHosts();

  // override in hosts does not work
  UEXPECT_THROW(resolver->Resolve("invalid", test_deadline),
                clients::dns::NotResolvedException);
  UEXPECT_THROW(resolver->Resolve("subdomain.invalid", test_deadline),
                clients::dns::NotResolvedException);

  // FQDN works
  UEXPECT_THROW(resolver->Resolve("invalid.", test_deadline),
                clients::dns::NotResolvedException);
  UEXPECT_THROW(resolver->Resolve("subdomain.invalid.", test_deadline),
                clients::dns::NotResolvedException);

  // we correctly identify domains
  EXPECT_PRED_FORMAT2(CheckAddrs,
                      resolver->Resolve("not-invalid", test_deadline),
                      (Expected{kNetV6String, kNetV4String}));

  const auto& counters = resolver->GetLookupSourceCounters();
  EXPECT_EQ(counters.file, 0);
  EXPECT_EQ(counters.cached, 0);
  EXPECT_EQ(counters.cached_stale, 0);
  EXPECT_EQ(counters.cached_failure, 0);
  EXPECT_EQ(counters.network, 1);
  EXPECT_EQ(counters.network_failure, 0);
}

UTEST(Resolver, FileUpdate) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  MockedResolver resolver{1000, 1};

  EXPECT_PRED_FORMAT2(CheckAddrs,
                      resolver->Resolve("mycomputer", test_deadline),
                      (Expected{"::1", "127.0.0.1"}));

  resolver.ReplaceHosts();

  EXPECT_PRED_FORMAT2(CheckAddrs,
                      resolver->Resolve("mycomputer", test_deadline),
                      (Expected{"::1", "127.0.0.1"}));

  resolver->ReloadHosts();

  EXPECT_PRED_FORMAT2(CheckAddrs,
                      resolver->Resolve("mycomputer", test_deadline),
                      (Expected{"127.0.0.2"}));

  const auto& counters = resolver->GetLookupSourceCounters();
  EXPECT_EQ(counters.file, 3);
  EXPECT_EQ(counters.cached, 0);
  EXPECT_EQ(counters.cached_stale, 0);
  EXPECT_EQ(counters.cached_failure, 0);
  EXPECT_EQ(counters.network, 0);
  EXPECT_EQ(counters.network_failure, 0);
}

UTEST(Resolver, CacheWorks) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  MockedResolver resolver{1000, 1};

  EXPECT_PRED_FORMAT2(CheckAddrs,
                      resolver->Resolve("not-mycomputer", test_deadline),
                      (Expected{kNetV6String, kNetV4String}));

  EXPECT_PRED_FORMAT2(CheckAddrs,
                      resolver->Resolve("not-mycomputer", test_deadline),
                      (Expected{kNetV6String, kNetV4String}));

  const auto& counters = resolver->GetLookupSourceCounters();
  EXPECT_EQ(counters.file, 0);
  EXPECT_EQ(counters.cached, 1);
  EXPECT_EQ(counters.cached_stale, 0);
  EXPECT_EQ(counters.cached_failure, 0);
  EXPECT_EQ(counters.network, 1);
  EXPECT_EQ(counters.network_failure, 0);
}

UTEST(Resolver, CacheOverflow) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  MockedResolver resolver{1000, 2};

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("first", test_deadline),
                      (Expected{kNetV6String, kNetV4String}));

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("second", test_deadline),
                      (Expected{kNetV6String, kNetV4String}));

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("first", test_deadline),
                      (Expected{kNetV6String, kNetV4String}));

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("third", test_deadline),
                      (Expected{kNetV6String, kNetV4String}));

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("first", test_deadline),
                      (Expected{kNetV6String, kNetV4String}));

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("second", test_deadline),
                      (Expected{kNetV6String, kNetV4String}));

  const auto& counters = resolver->GetLookupSourceCounters();
  EXPECT_EQ(counters.file, 0);
  EXPECT_EQ(counters.cached, 2);
  EXPECT_EQ(counters.cached_stale, 0);
  EXPECT_EQ(counters.cached_failure, 0);
  EXPECT_EQ(counters.network, 4);
  EXPECT_EQ(counters.network_failure, 0);
}

UTEST(Resolver, CacheStale) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  MockedResolver resolver{1, 1};

  utils::datetime::MockNowSet({});

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("first", test_deadline),
                      (Expected{kNetV6String, kNetV4String}));

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("first", test_deadline),
                      (Expected{kNetV6String, kNetV4String}));

  utils::datetime::MockSleep(std::chrono::seconds{3});

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("first", test_deadline),
                      (Expected{kNetV6String, kNetV4String}));

  const auto& counters = resolver->GetLookupSourceCounters();
  EXPECT_EQ(counters.file, 0);
  EXPECT_EQ(counters.cached, 1);
  EXPECT_EQ(counters.cached_stale, 1);
  EXPECT_EQ(counters.cached_failure, 0);
  EXPECT_GE(counters.network, 1);
  EXPECT_EQ(counters.network_failure, 0);
}

UTEST(Resolver, CacheFailures) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  MockedResolver resolver{1, 1};

  utils::datetime::MockNowSet({});

  UEXPECT_THROW(resolver->Resolve("fail", test_deadline),
                clients::dns::NotResolvedException);
  UEXPECT_THROW(resolver->Resolve("fail", test_deadline),
                clients::dns::NotResolvedException);

  utils::datetime::MockSleep(std::chrono::seconds{3});

  UEXPECT_THROW(resolver->Resolve("fail", test_deadline),
                clients::dns::NotResolvedException);

  const auto& counters = resolver->GetLookupSourceCounters();
  EXPECT_EQ(counters.file, 0);
  EXPECT_EQ(counters.cached, 0);
  EXPECT_EQ(counters.cached_stale, 0);
  EXPECT_EQ(counters.cached_failure, 1);
  EXPECT_GE(counters.network, 0);
  EXPECT_EQ(counters.network_failure, 2);
}

UTEST(Resolver, FileDoesNotCache) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  MockedResolver resolver{1, 1};

  EXPECT_PRED_FORMAT2(CheckAddrs,
                      resolver->Resolve("disappearing", test_deadline),
                      (Expected{"127.0.0.3"}));

  resolver.ReplaceHosts();
  resolver->ReloadHosts();

  EXPECT_PRED_FORMAT2(CheckAddrs,
                      resolver->Resolve("disappearing", test_deadline),
                      (Expected{kNetV6String, kNetV4String}));

  const auto& counters = resolver->GetLookupSourceCounters();
  EXPECT_EQ(counters.file, 1);
  EXPECT_EQ(counters.cached, 0);
  EXPECT_EQ(counters.cached_stale, 0);
  EXPECT_EQ(counters.cached_failure, 0);
  EXPECT_GE(counters.network, 1);
  EXPECT_EQ(counters.network_failure, 0);
}

UTEST(Resolver, FileOverridesCache) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  MockedResolver resolver{1, 1};

  UEXPECT_THROW(resolver->Resolve("fail", test_deadline),
                clients::dns::NotResolvedException);
  UEXPECT_THROW(resolver->Resolve("fail", test_deadline),
                clients::dns::NotResolvedException);

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("override", test_deadline),
                      (Expected{kNetV6String, kNetV4String}));
  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("override", test_deadline),
                      (Expected{kNetV6String, kNetV4String}));

  resolver.ReplaceHosts();
  resolver->ReloadHosts();

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("fail", test_deadline),
                      (Expected{"127.0.0.4"}));
  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("override", test_deadline),
                      (Expected{"127.0.0.5"}));

  const auto& counters = resolver->GetLookupSourceCounters();
  EXPECT_EQ(counters.file, 2);
  EXPECT_EQ(counters.cached, 1);
  EXPECT_EQ(counters.cached_stale, 0);
  EXPECT_EQ(counters.cached_failure, 1);
  EXPECT_GE(counters.network, 1);
  EXPECT_EQ(counters.network_failure, 1);
}

UTEST(Resolver, FirstUpdateCombines) {
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  MockedResolver resolver{1000, 1};
  resolver.reply_delay = std::chrono::milliseconds{50};

  auto first_ok = utils::Async(
      "first-ok", [&] { return resolver->Resolve("test", test_deadline); });
  auto second_ok = utils::Async(
      "second-ok", [&] { return resolver->Resolve("test", test_deadline); });
  auto first_fail = utils::Async(
      "first-fail", [&] { return resolver->Resolve("fail", test_deadline); });
  auto second_fail = utils::Async(
      "second_fail", [&] { return resolver->Resolve("fail", test_deadline); });

  EXPECT_PRED_FORMAT2(CheckAddrs, first_ok.Get(),
                      (Expected{kNetV6String, kNetV4String}));
  EXPECT_PRED_FORMAT2(CheckAddrs, second_ok.Get(),
                      (Expected{kNetV6String, kNetV4String}));
  UEXPECT_THROW(first_fail.Get(), clients::dns::NotResolvedException);
  UEXPECT_THROW(second_fail.Get(), clients::dns::NotResolvedException);

  const auto& counters = resolver->GetLookupSourceCounters();
  EXPECT_EQ(counters.file, 0);
  EXPECT_EQ(counters.cached, 1);
  EXPECT_EQ(counters.cached_stale, 0);
  EXPECT_EQ(counters.cached_failure, 1);
  EXPECT_EQ(counters.network, 1);
  EXPECT_EQ(counters.network_failure, 1);
}

USERVER_NAMESPACE_END
