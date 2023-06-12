#include <userver/utest/utest.hpp>

#include <storages/redis/client_redistest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using ScanTag = storages::redis::ScanTag;
template <ScanTag Tag>
class ScanType {
 public:
  static constexpr auto value = Tag;
};

template <typename T>
class RedisClientScanTest : public RedisClientTest {
  static constexpr ScanTag Tag = T::value;
  using Match = storages::redis::ScanOptionsBase::Match;
  using Count = storages::redis::ScanOptionsBase::Count;

 public:
  void SetUp() override;

  std::vector<std::string> GetActual();

  static std::vector<std::string> GetExpected() {
    std::vector<std::string> expected;
    std::regex rgx(pattern_cpp);
    std::smatch match;
    for (int i = 0; i < N; i++) {
      auto key = "key:" + std::to_string(i);
      if (std::regex_match(key, match, rgx)) expected.emplace_back(key);
    }
    std::sort(expected.begin(), expected.end());
    return expected;
  }

 private:
  static constexpr int N = 1000;
  static const std::string pattern;
  static const std::string pattern_cpp;

  static auto GetScanOptions() {
    static Match match(pattern);
    static Count count(10);
    static storages::redis::ScanOptionsTmpl<Tag> options(match, count);
    return options;
  }
};

template <typename T>
const std::string RedisClientScanTest<T>::pattern = "key:*1*";
template <typename T>
const std::string RedisClientScanTest<T>::pattern_cpp = "key:[0-9]*1[0-9]*";

template <>
void RedisClientScanTest<ScanType<ScanTag::kScan>>::SetUp() {
  RedisClientTest::SetUp();
  auto client = GetClient();
  for (int i = 0; i < N; i++)
    client->Set("key:" + std::to_string(i), "value", {}).Get();
}

template <>
std::vector<std::string>
RedisClientScanTest<ScanType<ScanTag::kScan>>::GetActual() {
  auto client = GetClient();
  std::vector<std::string> actual(
      client->Scan(0, GetScanOptions(), {}).GetAll());
  std::sort(actual.begin(), actual.end());
  return actual;
}

template <>
void RedisClientScanTest<ScanType<ScanTag::kHscan>>::SetUp() {
  RedisClientTest::SetUp();
  auto client = GetClient();
  for (int i = 0; i < N; i++)
    client->Hset("key", "key:" + std::to_string(i), "value", {}).Get();
}

template <>
std::vector<std::string>
RedisClientScanTest<ScanType<ScanTag::kHscan>>::GetActual() {
  auto client = GetClient();
  auto raw = client->Hscan("key", GetScanOptions(), {}).GetAll();
  std::vector<std::string> actual;
  std::transform(
      raw.begin(), raw.end(), std::back_inserter(actual),
      [](const std::pair<std::string, std::string>& p) { return p.first; });
  std::sort(actual.begin(), actual.end());
  return actual;
}

template <>
void RedisClientScanTest<ScanType<ScanTag::kSscan>>::SetUp() {
  RedisClientTest::SetUp();
  auto client = GetClient();
  for (int i = 0; i < N; i++)
    client->Sadd("key", "key:" + std::to_string(i), {}).Get();
}

template <>
std::vector<std::string>
RedisClientScanTest<ScanType<ScanTag::kSscan>>::GetActual() {
  auto client = GetClient();
  std::vector<std::string> actual(
      client->Sscan("key", GetScanOptions(), {}).GetAll());
  std::sort(actual.begin(), actual.end());
  return actual;
}

template <>
void RedisClientScanTest<ScanType<ScanTag::kZscan>>::SetUp() {
  RedisClientTest::SetUp();
  auto client = GetClient();
  for (int i = 0; i < N; i++)
    client->Zadd("key", i, "key:" + std::to_string(i), {}).Get();
}

template <>
std::vector<std::string>
RedisClientScanTest<ScanType<ScanTag::kZscan>>::GetActual() {
  auto client = GetClient();
  auto raw = client->Zscan("key", GetScanOptions(), {}).GetAll();
  std::vector<std::string> actual;
  std::transform(raw.begin(), raw.end(), std::back_inserter(actual),
                 [](const auto& p) { return p.member; });
  std::sort(actual.begin(), actual.end());
  return actual;
}

}  // namespace

using ScanTypes =
    testing::Types<ScanType<ScanTag::kScan>, ScanType<ScanTag::kHscan>,
                   ScanType<ScanTag::kSscan>, ScanType<ScanTag::kZscan>>;
TYPED_UTEST_SUITE(RedisClientScanTest, ScanTypes);

TYPED_UTEST(RedisClientScanTest, Scan) {
  auto actual = this->GetActual();
  auto expected = this->GetExpected();

  EXPECT_EQ(actual.size(), expected.size());
  EXPECT_EQ(actual, expected);
}

USERVER_NAMESPACE_END
