#include <storages/redis/client_redistest.hpp>

#include <userver/utest/utest.hpp>

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
    using Match = storages::redis::ScanOptionsGeneric::Match;
    using Count = storages::redis::ScanOptionsGeneric::Count;

public:
    void SetUp() override;

    std::vector<std::string> GetActual();

    static std::vector<std::string> GetExpected() {
        std::vector<std::string> expected;
        const utils::regex rgx(pattern_cpp);
        utils::match_results match;
        for (int i = 0; i < N; i++) {
            auto key = "key:" + std::to_string(i);
            if (utils::regex_match(key, match, rgx)) expected.emplace_back(key);
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
        static const storages::redis::ScanOptionsGeneric options(match, count);
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
    for (int i = 0; i < N; i++) client->Set("key:" + std::to_string(i), "value", {}).Get();
}

template <>
std::vector<std::string> RedisClientScanTest<ScanType<ScanTag::kScan>>::GetActual() {
    auto client = GetClient();
    std::vector<std::string> actual(client->Scan(0, GetScanOptions(), {}).GetAll());
    std::sort(actual.begin(), actual.end());
    return actual;
}

template <>
void RedisClientScanTest<ScanType<ScanTag::kHscan>>::SetUp() {
    RedisClientTest::SetUp();
    auto client = GetClient();
    for (int i = 0; i < N; i++) client->Hset("key", "key:" + std::to_string(i), "value", {}).Get();
}

template <>
std::vector<std::string> RedisClientScanTest<ScanType<ScanTag::kHscan>>::GetActual() {
    auto client = GetClient();
    auto raw = client->Hscan("key", GetScanOptions(), {}).GetAll();
    std::vector<std::string> actual;
    std::transform(
        raw.begin(),
        raw.end(),
        std::back_inserter(actual),
        [](const std::pair<std::string, std::string>& p) { return p.first; }
    );
    std::sort(actual.begin(), actual.end());
    return actual;
}

template <>
void RedisClientScanTest<ScanType<ScanTag::kSscan>>::SetUp() {
    RedisClientTest::SetUp();
    auto client = GetClient();
    for (int i = 0; i < N; i++) client->Sadd("key", "key:" + std::to_string(i), {}).Get();
}

template <>
std::vector<std::string> RedisClientScanTest<ScanType<ScanTag::kSscan>>::GetActual() {
    auto client = GetClient();
    std::vector<std::string> actual(client->Sscan("key", GetScanOptions(), {}).GetAll());
    std::sort(actual.begin(), actual.end());
    return actual;
}

template <>
void RedisClientScanTest<ScanType<ScanTag::kZscan>>::SetUp() {
    RedisClientTest::SetUp();
    auto client = GetClient();
    for (int i = 0; i < N; i++) client->Zadd("key", i, "key:" + std::to_string(i), {}).Get();
}

template <>
std::vector<std::string> RedisClientScanTest<ScanType<ScanTag::kZscan>>::GetActual() {
    auto client = GetClient();
    auto raw = client->Zscan("key", GetScanOptions(), {}).GetAll();
    std::vector<std::string> actual;
    std::transform(raw.begin(), raw.end(), std::back_inserter(actual), [](const auto& p) { return p.member; });
    std::sort(actual.begin(), actual.end());
    return actual;
}

}  // namespace

using ScanTypes = testing::
    Types<ScanType<ScanTag::kScan>, ScanType<ScanTag::kHscan>, ScanType<ScanTag::kSscan>, ScanType<ScanTag::kZscan>>;
TYPED_UTEST_SUITE(RedisClientScanTest, ScanTypes);

TYPED_UTEST(RedisClientScanTest, Scan) {
    auto actual = this->GetActual();
    auto expected = this->GetExpected();

    EXPECT_EQ(actual.size(), expected.size());
    EXPECT_EQ(actual, expected);
}

UTEST_F(RedisClientTest, SampleScan) {
    auto redis_client = GetClient();

    /// [Sample Scan usage]
    constexpr std::size_t kKeysWithSamePrefix = 100;
    for (std::size_t i = 0; i < kKeysWithSamePrefix; ++i) {
        redis_client->Set("the_key_" + std::to_string(i), "the_value", {}).Get();
        redis_client->Set("a_key_" + std::to_string(i), "a_value", {}).Get();
    }

    std::size_t matches_found = 0;
    for (const std::string& field : redis_client->Scan(0, storages::redis::Match{"the_key*"}, {})) {
        EXPECT_EQ(field.find("the_key"), 0) << "Does not start with 'the_key'";
        ++matches_found;
    }
    EXPECT_EQ(matches_found, kKeysWithSamePrefix);
    /// [Sample Scan usage]
}

UTEST_F(RedisClientTest, SampleSscan) {
    auto redis_client = GetClient();

    /// [Sample Sadd and Sscan usage]
    constexpr std::size_t kKeysWithSamePrefix = 100;
    static const std::string kKey = "key";
    for (std::size_t i = 0; i < kKeysWithSamePrefix; ++i) {
        redis_client->Sadd(kKey, "the_value_" + std::to_string(i), {}).Get();
        redis_client->Sadd(kKey, "a_value_" + std::to_string(i), {}).Get();
    }

    std::size_t matches_found = 0;
    for (const std::string& value : redis_client->Sscan(kKey, storages::redis::Match{"the_value*"}, {})) {
        EXPECT_EQ(value.find("the_value"), 0) << "Does not start with 'the_value'";
        ++matches_found;
    }
    EXPECT_EQ(matches_found, kKeysWithSamePrefix);
    /// [Sample Sadd and Sscan usage]
}

UTEST_F(RedisClientTest, SampleHscan) {
    auto redis_client = GetClient();

    /// [Sample Hscan usage]
    constexpr std::size_t kKeysWithSamePrefix = 100;
    static const std::string kKey = "key";
    for (std::size_t i = 0; i < kKeysWithSamePrefix; ++i) {
        redis_client->Hset(kKey, "the_field_" + std::to_string(i), "the_value", {}).Get();
        redis_client->Hset(kKey, "a_field_" + std::to_string(i), "a_value", {}).Get();
    }

    std::size_t matches_found = 0;
    for (const auto& [field, value] : redis_client->Hscan(kKey, storages::redis::Match{"the_field*"}, {})) {
        EXPECT_EQ(field.find("the_field"), 0) << "Does not start with 'the_field'";
        EXPECT_EQ(value, "the_value");
        ++matches_found;
    }
    EXPECT_EQ(matches_found, kKeysWithSamePrefix);
    /// [Sample Hscan usage]
}

UTEST_F(RedisClientTest, SampleZscan) {
    auto redis_client = GetClient();

    /// [Sample Zscan usage]
    constexpr std::size_t kKeysWithSamePrefix = 100;
    static const std::string kKey = "key";
    for (std::size_t i = 0; i < kKeysWithSamePrefix; ++i) {
        redis_client->Zadd(kKey, static_cast<double>(i), "the_value_" + std::to_string(i), {}).Get();
        redis_client->Zadd(kKey, static_cast<double>(i), "a_value_" + std::to_string(i), {}).Get();
    }

    std::size_t matches_found = 0;
    for (const storages::redis::MemberScore& ms : redis_client->Zscan(kKey, storages::redis::Match{"the_value*"}, {})) {
        const auto& [member, score] = ms;
        EXPECT_EQ(member.find("the_value"), 0) << "Does not start with 'the_value'";
        ++matches_found;
    }
    EXPECT_EQ(matches_found, kKeysWithSamePrefix);
    /// [Sample Zscan usage]
}

USERVER_NAMESPACE_END
