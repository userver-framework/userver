#include <string_view>
#include <vector>

#include <userver/clients/dns/config.hpp>
#include <userver/clients/dns/exception.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/utest/dns_server_mock.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/mock_now.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

static constexpr auto kTestHosts = R"(
127.0.0.1 localhost
::1 localhost
127.0.0.3 disappearing
)";

static constexpr auto kReplacementHosts = R"(
127.0.0.2 localhost
127.0.0.4 fail
127.0.0.5 override
)";

static const auto kNetV4Sockaddr = [] {
  engine::io::Sockaddr sockaddr;
  auto* sa = sockaddr.As<sockaddr_in>();
  sa->sin_family = AF_INET;
  sa->sin_addr.s_addr = htonl(0x4D583737);
  return sockaddr;
}();
constexpr static auto kNetV4String{"77.88.55.55"};

static const auto kNetV6Sockaddr = [] {
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
constexpr static auto kNetV6String{"2a02:6b8:a::a"};

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
              config.file_update_interval = kMaxTestWaitTime;
              config.network_timeout = kMaxTestWaitTime;
              config.network_attempts = 1;
              config.cache_max_reply_ttl = std::chrono::seconds{cache_max_ttl};
              config.cache_failure_ttl = std::chrono::seconds{cache_max_ttl},
              config.cache_ways = 1;
              config.cache_size_per_way = cache_size_per_way;
              config.network_custom_servers = {server_mock.GetServerAddress()};
              return config;
            }()} {}

  clients::dns::Resolver* operator->() { return &resolver; }

  void ReplaceHosts() {
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
  if (addrs.size() != expected.size()) {
    return ::testing::AssertionFailure()
           << addrs_text << " returned wrong number of addresses: expected "
           << expected.size() << ", got " << addrs.size();
  }

  for (size_t i = 0; i < addrs.size(); ++i) {
    const auto addr_str = addrs[i].PrimaryAddressString();
    if (addr_str != expected[i]) {
      return ::testing::AssertionFailure()
             << addrs_text << " has unexpected address at position " << i
             << ": expected " << expected[i] << ", got " << addr_str;
    }
  }
  return ::testing::AssertionSuccess();
}

}  // namespace

UTEST(Resolver, Smoke) {
  MockedResolver resolver{1000, 1};

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("localhost"),
                      (Expected{"::1", "127.0.0.1"}));

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("not-localhost"),
                      (Expected{kNetV6String, kNetV4String}));

  EXPECT_THROW(resolver->Resolve("fail"), clients::dns::NotResolvedException);

  const auto& counters = resolver->GetLookupSourceCounters();
  EXPECT_EQ(counters.file, 1);
  EXPECT_EQ(counters.cached, 0);
  EXPECT_EQ(counters.cached_stale, 0);
  EXPECT_EQ(counters.cached_failure, 0);
  EXPECT_EQ(counters.network, 1);
  EXPECT_EQ(counters.network_failure, 1);
}

UTEST(Resolver, FileUpdate) {
  MockedResolver resolver{1000, 1};

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("localhost"),
                      (Expected{"::1", "127.0.0.1"}));

  resolver.ReplaceHosts();

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("localhost"),
                      (Expected{"::1", "127.0.0.1"}));

  resolver->ReloadHosts();

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("localhost"),
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
  MockedResolver resolver{1000, 1};

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("not-localhost"),
                      (Expected{kNetV6String, kNetV4String}));

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("not-localhost"),
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
  MockedResolver resolver{1000, 2};

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("first"),
                      (Expected{kNetV6String, kNetV4String}));

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("second"),
                      (Expected{kNetV6String, kNetV4String}));

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("first"),
                      (Expected{kNetV6String, kNetV4String}));

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("third"),
                      (Expected{kNetV6String, kNetV4String}));

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("first"),
                      (Expected{kNetV6String, kNetV4String}));

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("second"),
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
  MockedResolver resolver{1, 1};

  utils::datetime::MockNowSet({});

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("first"),
                      (Expected{kNetV6String, kNetV4String}));

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("first"),
                      (Expected{kNetV6String, kNetV4String}));

  utils::datetime::MockSleep(std::chrono::seconds{3});

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("first"),
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
  MockedResolver resolver{1, 1};

  utils::datetime::MockNowSet({});

  EXPECT_THROW(resolver->Resolve("fail"), clients::dns::NotResolvedException);
  EXPECT_THROW(resolver->Resolve("fail"), clients::dns::NotResolvedException);

  utils::datetime::MockSleep(std::chrono::seconds{3});

  EXPECT_THROW(resolver->Resolve("fail"), clients::dns::NotResolvedException);

  const auto& counters = resolver->GetLookupSourceCounters();
  EXPECT_EQ(counters.file, 0);
  EXPECT_EQ(counters.cached, 0);
  EXPECT_EQ(counters.cached_stale, 0);
  EXPECT_EQ(counters.cached_failure, 1);
  EXPECT_GE(counters.network, 0);
  EXPECT_EQ(counters.network_failure, 2);
}

UTEST(Resolver, FileDoesNotCache) {
  MockedResolver resolver{1, 1};

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("disappearing"),
                      (Expected{"127.0.0.3"}));

  resolver.ReplaceHosts();
  resolver->ReloadHosts();

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("disappearing"),
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
  MockedResolver resolver{1, 1};

  EXPECT_THROW(resolver->Resolve("fail"), clients::dns::NotResolvedException);
  EXPECT_THROW(resolver->Resolve("fail"), clients::dns::NotResolvedException);

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("override"),
                      (Expected{kNetV6String, kNetV4String}));
  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("override"),
                      (Expected{kNetV6String, kNetV4String}));

  resolver.ReplaceHosts();
  resolver->ReloadHosts();

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("fail"),
                      (Expected{"127.0.0.4"}));
  EXPECT_PRED_FORMAT2(CheckAddrs, resolver->Resolve("override"),
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
  MockedResolver resolver{1000, 1};
  resolver.reply_delay = std::chrono::milliseconds{50};

  auto first_ok =
      utils::Async("first-ok", [&] { return resolver->Resolve("test"); });
  auto second_ok =
      utils::Async("second-ok", [&] { return resolver->Resolve("test"); });
  auto first_fail =
      utils::Async("first-fail", [&] { return resolver->Resolve("fail"); });
  auto second_fail =
      utils::Async("second_fail", [&] { return resolver->Resolve("fail"); });

  EXPECT_PRED_FORMAT2(CheckAddrs, first_ok.Get(),
                      (Expected{kNetV6String, kNetV4String}));
  EXPECT_PRED_FORMAT2(CheckAddrs, second_ok.Get(),
                      (Expected{kNetV6String, kNetV4String}));
  EXPECT_THROW(first_fail.Get(), clients::dns::NotResolvedException);
  EXPECT_THROW(second_fail.Get(), clients::dns::NotResolvedException);

  const auto& counters = resolver->GetLookupSourceCounters();
  EXPECT_EQ(counters.file, 0);
  EXPECT_EQ(counters.cached, 1);
  EXPECT_EQ(counters.cached_stale, 0);
  EXPECT_EQ(counters.cached_failure, 1);
  EXPECT_EQ(counters.network, 1);
  EXPECT_EQ(counters.network_failure, 1);
}

USERVER_NAMESPACE_END
