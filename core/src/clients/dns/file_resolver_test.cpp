#include <userver/utest/utest.hpp>

#include <string_view>
#include <vector>

#include <clients/dns/file_resolver.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/blocking/write.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr auto kTestHosts = R"(
127.0.0.1 my-localhost # my own localhost
127.0.0.2 also-my-localhost my-localhost # complex # comment

::2 also-my-localhost
### what follows is more hosts
this is an invalid string that must be ignored

1.2.3.4::5 invalid-addr
1:::1 partly-valid-addr invalid-addr
1::1 partly-valid-addr

5.5.5.5 host1 host2
5.5.5.6 host2 host1
)";

constexpr auto kReplacementHosts = R"(
127.0.0.1 localhost
::1 localhost
)";

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

UTEST(FileResolver, Smoke) {
  auto hosts_file = fs::blocking::TempFile::Create();
  fs::blocking::RewriteFileContents(hosts_file.GetPath(), kTestHosts);

  clients::dns::FileResolver resolver(engine::current_task::GetTaskProcessor(),
                                      hosts_file.GetPath(),
                                      utest::kMaxTestWaitTime);

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver.Resolve("localhost"), (Expected{}));
  EXPECT_PRED_FORMAT2(CheckAddrs, resolver.Resolve("my-localhost"),
                      (Expected{"127.0.0.1", "127.0.0.2"}));
  EXPECT_PRED_FORMAT2(CheckAddrs, resolver.Resolve("also-my-localhost"),
                      (Expected{"::2", "127.0.0.2"}));
  EXPECT_PRED_FORMAT2(CheckAddrs, resolver.Resolve("complex"), (Expected{}));
  EXPECT_PRED_FORMAT2(CheckAddrs, resolver.Resolve("comment"), (Expected{}));
  EXPECT_PRED_FORMAT2(CheckAddrs, resolver.Resolve("this"), (Expected{}));
  EXPECT_PRED_FORMAT2(CheckAddrs, resolver.Resolve("invalid-addr"),
                      (Expected{}));
  EXPECT_PRED_FORMAT2(CheckAddrs, resolver.Resolve("partly-valid-addr"),
                      (Expected{"1::1"}));
  EXPECT_PRED_FORMAT2(CheckAddrs, resolver.Resolve("host1"),
                      (Expected{"5.5.5.5", "5.5.5.6"}));
  EXPECT_PRED_FORMAT2(CheckAddrs, resolver.Resolve("host2"),
                      (Expected{"5.5.5.5", "5.5.5.6"}));
  EXPECT_PRED_FORMAT2(CheckAddrs, resolver.Resolve("nonexistent"),
                      (Expected{}));

  fs::blocking::RewriteFileContents(hosts_file.GetPath(), kReplacementHosts);
  resolver.ReloadHosts();

  EXPECT_PRED_FORMAT2(CheckAddrs, resolver.Resolve("localhost"),
                      (Expected{"::1", "127.0.0.1"}));
  EXPECT_PRED_FORMAT2(CheckAddrs, resolver.Resolve("my-localhost"),
                      (Expected{}));
  EXPECT_PRED_FORMAT2(CheckAddrs, resolver.Resolve("nonexistent"),
                      (Expected{}));
}

USERVER_NAMESPACE_END
