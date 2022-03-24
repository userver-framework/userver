#include <userver/utest/utest.hpp>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/formats/json/serialize.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct DummyConfig final {
  int foo;
  std::string bar;

  static DummyConfig Parse(const dynamic_config::DocsMap&) { return {}; }
};

DummyConfig Parse(const formats::json::Value& value,
                  formats::parse::To<DummyConfig>) {
  return {value["foo"].As<int>(), value["bar"].As<std::string>()};
}

constexpr dynamic_config::Key<DummyConfig::Parse> kDummyConfig;

int ParseIntConfig(const dynamic_config::DocsMap&) { return {}; }

constexpr dynamic_config::Key<ParseIntConfig> kIntConfig;

bool ParseBoolConfig(const dynamic_config::DocsMap&) { return {}; }

constexpr dynamic_config::Key<ParseBoolConfig> kBoolConfig;

struct DummyConfigWrapper final {
  int GetFoo() const { return config[kDummyConfig].foo; }

  dynamic_config::Snapshot config;
};

class DynamicConfigTest : public testing::Test {
 protected:
  const dynamic_config::StorageMock storage_{{kDummyConfig, {42, "what"}},
                                             {kIntConfig, 5}};
  const dynamic_config::Source source_ = storage_.GetSource();
  const dynamic_config::Snapshot config_ = source_.GetSnapshot();
};

UTEST_F(DynamicConfigTest, GetExistingConfigClass) {
  const auto& my_config = config_[kDummyConfig];
  EXPECT_EQ(my_config.foo, 42);
  EXPECT_EQ(my_config.bar, "what");
}

UTEST_F(DynamicConfigTest, GetExistingConfigTrivial) {
  const auto& int_config = config_[kIntConfig];
  EXPECT_EQ(int_config, 5);
}

UTEST_F(DynamicConfigTest, GetMissingConfig) {
  UEXPECT_THROW(config_[kBoolConfig], std::logic_error);
}

UTEST_F(DynamicConfigTest, Snapshot) {
  const auto snapshot = source_.GetSnapshot();
  const auto& my_config = snapshot[kDummyConfig];
  EXPECT_EQ(my_config.foo, 42);
  EXPECT_EQ(my_config.bar, "what");
}

UTEST_F(DynamicConfigTest, ConfigCopyable) {
  DummyConfigWrapper wrapper{config_};  // Config is copied
  EXPECT_EQ(wrapper.GetFoo(), 42);
}

UTEST_F(DynamicConfigTest, VariableSnapshotPtr) {
  const auto my_config = source_.GetSnapshot(kDummyConfig);
  EXPECT_EQ(my_config->foo, 42);
  EXPECT_EQ(my_config->bar, "what");
}

UTEST_F(DynamicConfigTest, Copy) { EXPECT_EQ(source_.GetCopy(kIntConfig), 5); }

struct ByConstructor final {
  int foo{42};

  explicit ByConstructor(const dynamic_config::DocsMap&) {}

  ByConstructor() = default;
};

UTEST(DynamicConfig, TheOldWay) {
  // Only for the purposes of testing, don't use in production code
  dynamic_config::Key<dynamic_config::impl::ParseByConstructor<ByConstructor>>
      key;
  const dynamic_config::StorageMock storage{{key, {}}};

  const auto config = storage.GetSource().GetSnapshot();
  EXPECT_EQ(config.Get<ByConstructor>().foo, 42);
}

class DummyClient final {
 public:
  explicit DummyClient(dynamic_config::Source config) : config_(config) {}

  void DoStuff() {
    const auto snapshot = config_.GetSnapshot();
    if (snapshot[kDummyConfig].foo != 42) {
      throw std::runtime_error("What?");
    }
  }

 private:
  dynamic_config::Source config_;
};

/// [Sample StorageMock usage]
class DummyClient;

std::string DummyFunction(const dynamic_config::Snapshot& config) {
  return config[kDummyConfig].bar;
}

UTEST(DynamicConfig, Snippet) {
  // The 'StorageMock' will only contain the specified configs, and nothing more
  dynamic_config::StorageMock storage{
      {kDummyConfig, {42, "what"}},
      {kIntConfig, 5},
  };

  EXPECT_EQ(DummyFunction(storage.GetSnapshot()), "what");

  // 'DummyClient' stores 'dynamic_config::Source' for access to latest configs
  DummyClient client{storage.GetSource()};
  UEXPECT_NO_THROW(client.DoStuff());

  storage.Extend({{kDummyConfig, {-10000, "invalid"}}});
  UEXPECT_THROW(client.DoStuff(), std::runtime_error);
}
/// [Sample StorageMock usage]

UTEST(DynamicConfig, Extend) {
  std::vector<dynamic_config::KeyValue> vars1{{kIntConfig, 5},
                                              {kBoolConfig, true}};
  std::vector<dynamic_config::KeyValue> vars2{{kIntConfig, 10},
                                              {kDummyConfig, {42, "what"}}};

  dynamic_config::StorageMock storage{vars1};
  storage.Extend(vars2);

  const auto config = storage.GetSnapshot();
  EXPECT_EQ(config[kIntConfig], 10);
  EXPECT_EQ(config[kBoolConfig], true);
  EXPECT_EQ(config[kDummyConfig].foo, 42);
}

/// [StorageMock from JSON]
const auto kJson = formats::json::FromString(R"( {"foo": 42, "bar": "what"} )");

UTEST(DynamicConfig, FromJson) {
  dynamic_config::StorageMock storage{
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

dynamic_config::StorageMock MakeFooConfig() {
  return {
      {kDummyConfig, {42, std::string(kLongString)}},
      {kIntConfig, 5},
  };
}

std::vector<dynamic_config::KeyValue> MakeBarConfig() {
  return {{kBoolConfig, false}};
}

UTEST(DynamicConfig, Extend2) {
  auto storage = MakeFooConfig();
  storage.Extend(MakeBarConfig());

  const auto config = storage.GetSnapshot();
  EXPECT_EQ(config[kDummyConfig].foo, 42);
  EXPECT_EQ(config[kDummyConfig].bar, kLongString);
  EXPECT_EQ(config[kIntConfig], 5);
}

}  // namespace

USERVER_NAMESPACE_END
