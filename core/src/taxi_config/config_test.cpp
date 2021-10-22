#include <userver/utest/utest.hpp>

#include <userver/formats/json/serialize.hpp>
#include <userver/taxi_config/snapshot.hpp>
#include <userver/taxi_config/source.hpp>
#include <userver/taxi_config/storage_mock.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct DummyConfig final {
  int foo;
  std::string bar;

  static DummyConfig Parse(const taxi_config::DocsMap&) { return {}; }
};

DummyConfig Parse(const formats::json::Value& value,
                  formats::parse::To<DummyConfig>) {
  return {value["foo"].As<int>(), value["bar"].As<std::string>()};
}

constexpr taxi_config::Key<DummyConfig::Parse> kDummyConfig;

int ParseIntConfig(const taxi_config::DocsMap&) { return {}; }

constexpr taxi_config::Key<ParseIntConfig> kIntConfig;

bool ParseBoolConfig(const taxi_config::DocsMap&) { return {}; }

constexpr taxi_config::Key<ParseBoolConfig> kBoolConfig;

struct DummyConfigWrapper final {
  int GetFoo() const { return config[kDummyConfig].foo; }

  taxi_config::Snapshot config;
};

class TaxiConfigTest : public testing::Test {
 protected:
  const taxi_config::StorageMock storage_{{kDummyConfig, {42, "what"}},
                                          {kIntConfig, 5}};
  const taxi_config::Source source_ = storage_.GetSource();
  const taxi_config::Snapshot config_ = source_.GetSnapshot();
};

UTEST_F(TaxiConfigTest, GetExistingConfigClass) {
  const auto& my_config = config_[kDummyConfig];
  EXPECT_EQ(my_config.foo, 42);
  EXPECT_EQ(my_config.bar, "what");
}

UTEST_F(TaxiConfigTest, GetExistingConfigTrivial) {
  const auto& int_config = config_[kIntConfig];
  EXPECT_EQ(int_config, 5);
}

UTEST_F(TaxiConfigTest, GetMissingConfig) {
  EXPECT_THROW(config_[kBoolConfig], std::logic_error);
}

UTEST_F(TaxiConfigTest, Snapshot) {
  const auto snapshot = source_.GetSnapshot();
  const auto& my_config = snapshot[kDummyConfig];
  EXPECT_EQ(my_config.foo, 42);
  EXPECT_EQ(my_config.bar, "what");
}

UTEST_F(TaxiConfigTest, ConfigCopyable) {
  DummyConfigWrapper wrapper{config_};  // Config is copied
  EXPECT_EQ(wrapper.GetFoo(), 42);
}

UTEST_F(TaxiConfigTest, VariableSnapshotPtr) {
  const auto my_config = source_.GetSnapshot(kDummyConfig);
  EXPECT_EQ(my_config->foo, 42);
  EXPECT_EQ(my_config->bar, "what");
}

UTEST_F(TaxiConfigTest, Copy) { EXPECT_EQ(source_.GetCopy(kIntConfig), 5); }

struct ByConstructor final {
  int foo{42};

  explicit ByConstructor(const taxi_config::DocsMap&) {}

  ByConstructor() = default;
};

UTEST(TaxiConfig, TheOldWay) {
  // Only for the purposes of testing, don't use in production code
  taxi_config::Key<taxi_config::impl::ParseByConstructor<ByConstructor>> key;
  const taxi_config::StorageMock storage{{key, {}}};

  const auto config = storage.GetSource().GetSnapshot();
  EXPECT_EQ(config.Get<ByConstructor>().foo, 42);
}

class DummyClient final {
 public:
  explicit DummyClient(taxi_config::Source config) : config_(config) {}

  void DoStuff() {
    const auto snapshot = config_.GetSnapshot();
    if (snapshot[kDummyConfig].foo != 42) {
      throw std::runtime_error("What?");
    }
  }

 private:
  taxi_config::Source config_;
};

/// [Sample StorageMock usage]
class DummyClient;

std::string DummyFunction(const taxi_config::Snapshot& config) {
  return config[kDummyConfig].bar;
}

UTEST(TaxiConfig, Snippet) {
  // The 'StorageMock' will only contain the specified configs, and nothing more
  taxi_config::StorageMock storage{
      {kDummyConfig, {42, "what"}},
      {kIntConfig, 5},
  };

  EXPECT_EQ(DummyFunction(storage.GetSnapshot()), "what");

  // 'DummyClient' stores 'taxi_config::Source' for access to latest configs
  DummyClient client{storage.GetSource()};
  EXPECT_NO_THROW(client.DoStuff());

  storage.Extend({{kDummyConfig, {-10000, "invalid"}}});
  EXPECT_ANY_THROW(client.DoStuff());
}
/// [Sample StorageMock usage]

UTEST(TaxiConfig, Extend) {
  std::vector<taxi_config::KeyValue> vars1{{kIntConfig, 5},
                                           {kBoolConfig, true}};
  std::vector<taxi_config::KeyValue> vars2{{kIntConfig, 10},
                                           {kDummyConfig, {42, "what"}}};

  taxi_config::StorageMock storage{vars1};
  storage.Extend(vars2);

  const auto config = storage.GetSnapshot();
  EXPECT_EQ(config[kIntConfig], 10);
  EXPECT_EQ(config[kBoolConfig], true);
  EXPECT_EQ(config[kDummyConfig].foo, 42);
}

/// [StorageMock from JSON]
const auto kJson = formats::json::FromString(R"( {"foo": 42, "bar": "what"} )");

UTEST(TaxiConfig, FromJson) {
  taxi_config::StorageMock storage{
      {kDummyConfig, kJson},
      {kIntConfig, 5},
  };

  const auto config = storage.GetSnapshot();
  EXPECT_EQ(config[kDummyConfig].foo, 42);
  EXPECT_EQ(config[kDummyConfig].bar, "what");
  EXPECT_EQ(config[kIntConfig], 5);
}
/// [StorageMock from JSON]

constexpr std::string_view kLongString =
    "Some long long long long long long long long long string";

taxi_config::StorageMock MakeFooConfig() {
  return {
      {kDummyConfig, {42, std::string(kLongString)}},
      {kIntConfig, 5},
  };
}

std::vector<taxi_config::KeyValue> MakeBarConfig() {
  return {{kBoolConfig, false}};
}

UTEST(TaxiConfig, Extend2) {
  auto storage = MakeFooConfig();
  storage.Extend(MakeBarConfig());

  const auto config = storage.GetSnapshot();
  EXPECT_EQ(config[kDummyConfig].foo, 42);
  EXPECT_EQ(config[kDummyConfig].bar, kLongString);
  EXPECT_EQ(config[kIntConfig], 5);
}

}  // namespace

USERVER_NAMESPACE_END
