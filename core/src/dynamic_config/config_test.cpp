#include <userver/utest/utest.hpp>

#include <vector>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/source.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/formats/json/serialize.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

/// [struct config hpp]
// hpp
struct SampleStructConfig final {
  bool is_foo_enabled;
  std::chrono::milliseconds bar_period;
};

extern const dynamic_config::Key<SampleStructConfig> kSampleStructConfig;
/// [struct config hpp]

/// [struct config cpp]
// cpp
// Parser for a type must be defined in the same namespace as the type itself.
// In this example, JSON parser is only needed for dynamic config, so declaring
// it in hpp is not strictly necessary.
SampleStructConfig Parse(const formats::json::Value& value,
                         formats::parse::To<SampleStructConfig>) {
  return SampleStructConfig{
      /*is_foo_enabled=*/value["is_foo_enabled"].As<bool>(),
      /*bar_period=*/value["bar_period_ms"].As<std::chrono::milliseconds>(),
  };
}

const dynamic_config::Key<SampleStructConfig> kSampleStructConfig{
    "SAMPLE_STRUCT_CONFIG", dynamic_config::DefaultAsJsonString{R"(
  {
    "is_foo_enabled": false,
    "bar_period_ms": 42000
  }
)"}};
/// [struct config cpp]

UTEST(DynamicConfig, SampleStructConfig) {
  const auto& config = dynamic_config::GetDefaultSnapshot();
  EXPECT_EQ(config[kSampleStructConfig].is_foo_enabled, false);
  EXPECT_EQ(config[kSampleStructConfig].bar_period, 42s);
}

struct DummyConfig final {
  int foo;
  std::string bar;
};

DummyConfig Parse(const formats::json::Value& value,
                  formats::parse::To<DummyConfig>) {
  return {value["foo"].As<int>(), value["bar"].As<std::string>()};
}

const dynamic_config::Key kDummyConfig{dynamic_config::ConstantConfig{},
                                       DummyConfig{42, "what"}};

const dynamic_config::Key kIntConfig{dynamic_config::ConstantConfig{}, 0};

const dynamic_config::Key kBoolConfig{dynamic_config::ConstantConfig{}, false};

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
  const DummyConfigWrapper wrapper{config_};  // Config is copied
  EXPECT_EQ(wrapper.GetFoo(), 42);
}

UTEST_F(DynamicConfigTest, VariableSnapshotPtr) {
  const auto my_config = source_.GetSnapshot(kDummyConfig);
  EXPECT_EQ(my_config->foo, 42);
  EXPECT_EQ(my_config->bar, "what");
}

UTEST_F(DynamicConfigTest, Copy) { EXPECT_EQ(source_.GetCopy(kIntConfig), 5); }

struct OldConfig final {
  static const dynamic_config::Key<OldConfig> kDeprecatedKey;

  int foo{42};
};

const dynamic_config::Key<OldConfig> OldConfig::kDeprecatedKey{
    dynamic_config::ConstantConfig{}, OldConfig{}};

UTEST(DynamicConfig, TheOldWay) {
  const dynamic_config::StorageMock storage{{OldConfig::kDeprecatedKey, {}}};

  const auto config = storage.GetSource().GetSnapshot();
  EXPECT_EQ(config.Get<OldConfig>().foo, 42);
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
  const std::vector<dynamic_config::KeyValue> vars1{
      {kIntConfig, 5},
      {kBoolConfig, true},
  };
  const std::vector<dynamic_config::KeyValue> vars2{
      {kIntConfig, 10},
      {kDummyConfig, {42, "what"}},
  };

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
  const dynamic_config::StorageMock storage{
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

class Subscriber final {
 public:
  void OnConfigUpdate(const dynamic_config::Snapshot&) { counter_++; }
  int GetCounter() const { return counter_; }

 private:
  int counter_{};
};

[[maybe_unused]] bool operator==(const DummyConfig& lhs,
                                 const DummyConfig& rhs) {
  return lhs.foo == rhs.foo && lhs.bar == rhs.bar;
}

UTEST(DynamicConfig, SingleSubscription) {
  const std::vector<dynamic_config::KeyValue> vars1{
      {kDummyConfig, {0, "bar"}}, {kIntConfig, 1}, {kBoolConfig, false}};
  const std::vector<dynamic_config::KeyValue> vars2{
      {kDummyConfig, {0, "foo"}}, {kIntConfig, 2}, {kBoolConfig, false}};
  const std::vector<dynamic_config::KeyValue> vars3{
      {kDummyConfig, {1, "foo"}}, {kIntConfig, 2}, {kBoolConfig, true}};

  dynamic_config::StorageMock storage;
  auto source = storage.GetSource();
  Subscriber subscribers[3];
  concurrent::AsyncEventSubscriberScope scopes[3];

  scopes[0] = source.UpdateAndListen(&subscribers[0], "",
                                     &Subscriber::OnConfigUpdate, kDummyConfig);
  scopes[1] = source.UpdateAndListen(&subscribers[1], "",
                                     &Subscriber::OnConfigUpdate, kIntConfig);
  scopes[2] = source.UpdateAndListen(&subscribers[2], "",
                                     &Subscriber::OnConfigUpdate, kBoolConfig);

  EXPECT_EQ(subscribers[0].GetCounter(), 1);
  EXPECT_EQ(subscribers[1].GetCounter(), 1);
  EXPECT_EQ(subscribers[2].GetCounter(), 1);

  storage.Extend(vars1);
  EXPECT_EQ(subscribers[0].GetCounter(), 2);
  EXPECT_EQ(subscribers[1].GetCounter(), 2);
  EXPECT_EQ(subscribers[2].GetCounter(), 2);

  storage.Extend(vars1);
  EXPECT_EQ(subscribers[0].GetCounter(), 2);
  EXPECT_EQ(subscribers[1].GetCounter(), 2);
  EXPECT_EQ(subscribers[2].GetCounter(), 2);

  storage.Extend(vars2);
  EXPECT_EQ(subscribers[0].GetCounter(), 3);
  EXPECT_EQ(subscribers[1].GetCounter(), 3);
  EXPECT_EQ(subscribers[2].GetCounter(), 2);

  storage.Extend(vars3);
  EXPECT_EQ(subscribers[0].GetCounter(), 4);
  EXPECT_EQ(subscribers[1].GetCounter(), 3);
  EXPECT_EQ(subscribers[2].GetCounter(), 3);
}

UTEST(DynamicConfig, GetEventChannel) {
  const std::vector<dynamic_config::KeyValue> vars1{
      {kIntConfig, 1},
      {kBoolConfig, false},
  };
  const std::vector<dynamic_config::KeyValue> vars2{
      {kIntConfig, 1},
      {kBoolConfig, true},
  };
  const std::vector<dynamic_config::KeyValue> vars3{
      {kIntConfig, 2},
      {kBoolConfig, true},
  };

  dynamic_config::StorageMock storage;
  storage.Extend(vars1);
  auto source = storage.GetSource();
  Subscriber subscribers[4];
  concurrent::AsyncEventSubscriberScope scopes[4];

  scopes[0] =
      source.UpdateAndListen(&subscribers[0], "", &Subscriber::OnConfigUpdate);
  scopes[1] = source.UpdateAndListen(&subscribers[1], "",
                                     &Subscriber::OnConfigUpdate, kIntConfig);
  scopes[2] = source.UpdateAndListen(&subscribers[2], "",
                                     &Subscriber::OnConfigUpdate, kBoolConfig);

  storage.Extend(vars2);
  EXPECT_EQ(subscribers[0].GetCounter(), 2);
  EXPECT_EQ(subscribers[1].GetCounter(), 1);
  EXPECT_EQ(subscribers[2].GetCounter(), 2);

  auto& mainChannel = source.GetEventChannel();
  scopes[3] =
      mainChannel.AddListener(concurrent::FunctionId(&subscribers[3]), "",
                              [&](const dynamic_config::Snapshot& snapshot) {
                                subscribers[3].OnConfigUpdate(snapshot);
                                subscribers[3].OnConfigUpdate(snapshot);
                              });

  storage.Extend(vars3);
  EXPECT_EQ(subscribers[0].GetCounter(), 3);
  EXPECT_EQ(subscribers[1].GetCounter(), 2);
  EXPECT_EQ(subscribers[2].GetCounter(), 2);
  EXPECT_EQ(subscribers[3].GetCounter(), 2);
}

UTEST(DynamicConfig, SubsetSubscription) {
  const std::vector<dynamic_config::KeyValue> vars1{
      {kDummyConfig, {0, "bar"}},
      {kIntConfig, 1},
      {kBoolConfig, false},
  };
  const std::vector<dynamic_config::KeyValue> vars2{
      {kDummyConfig, {0, "foo"}},
      {kIntConfig, 1},
      {kBoolConfig, true},
  };
  const std::vector<dynamic_config::KeyValue> vars3{
      {kDummyConfig, {0, "foo"}},
      {kIntConfig, 0},
      {kBoolConfig, true},
  };

  dynamic_config::StorageMock storage;
  auto source = storage.GetSource();
  Subscriber subscribers[2];
  concurrent::AsyncEventSubscriberScope scopes[2];

  scopes[0] =
      source.UpdateAndListen(&subscribers[0], "", &Subscriber::OnConfigUpdate,
                             kIntConfig, kBoolConfig);
  scopes[1] =
      source.UpdateAndListen(&subscribers[1], "", &Subscriber::OnConfigUpdate,
                             kDummyConfig, kBoolConfig);

  EXPECT_EQ(subscribers[0].GetCounter(), 1);
  EXPECT_EQ(subscribers[1].GetCounter(), 1);

  storage.Extend(vars1);
  EXPECT_EQ(subscribers[0].GetCounter(), 2);
  EXPECT_EQ(subscribers[1].GetCounter(), 2);

  storage.Extend(vars2);
  EXPECT_EQ(subscribers[0].GetCounter(), 3);
  EXPECT_EQ(subscribers[1].GetCounter(), 3);

  storage.Extend(vars3);
  EXPECT_EQ(subscribers[0].GetCounter(), 4);
  EXPECT_EQ(subscribers[1].GetCounter(), 3);

  storage.Extend(vars3);
  EXPECT_EQ(subscribers[0].GetCounter(), 4);
  EXPECT_EQ(subscribers[1].GetCounter(), 3);
}

class CustomSubscriber final {
 public:
  void OnConfigUpdate(const dynamic_config::Diff&) { counter_++; }
  int GetCounter() const { return counter_; }

 private:
  int counter_{};
};

UTEST(DynamicConfig, CustomSubscription) {
  dynamic_config::StorageMock storage;
  auto source = storage.GetSource();
  CustomSubscriber subscriber;

  auto scope = source.UpdateAndListen(&subscriber, "",
                                      &CustomSubscriber::OnConfigUpdate);
  EXPECT_EQ(subscriber.GetCounter(), 1);

  storage.Extend({});
  EXPECT_EQ(subscriber.GetCounter(), 2);
}

class ConfigSubscriber final {
 public:
  /*! [Custom subscription for dynamic config update] */
  void OnConfigUpdate(const dynamic_config::Diff& diff_data) {
    if (!diff_data.previous) return;

    const auto& previous = *diff_data.previous;
    const auto& current = diff_data.current;
    if (previous[kBoolConfig] != current[kBoolConfig]) {
      OnBoolConfigChanged();
    }
    if (previous[kDummyConfig].foo != current[kDummyConfig].foo) {
      OnFooChanged();
    }
  }
  void OnBoolConfigChanged() { bool_config_change_counter_++; }
  void OnFooChanged() { foo_interesting_event_counter_++; }
  /*! [Custom subscription for dynamic config update] */

  std::size_t GetBoolConfigChangeCounter() const {
    return bool_config_change_counter_;
  }
  std::size_t GetFooInterestingEventCounter() const {
    return foo_interesting_event_counter_;
  }

 private:
  std::size_t bool_config_change_counter_{};
  std::size_t foo_interesting_event_counter_{};
};

UTEST(DynamicConfigSubscription, Snippet) {
  dynamic_config::StorageMock storage;
  auto source = storage.GetSource();
  ConfigSubscriber subscriber;

  auto scope = source.UpdateAndListen(&subscriber, "",
                                      &ConfigSubscriber::OnConfigUpdate);

  storage.Extend(
      {{kIntConfig, 1}, {kBoolConfig, false}, {kDummyConfig, {1, "bar"}}});
  EXPECT_EQ(subscriber.GetBoolConfigChangeCounter(), 0);
  EXPECT_EQ(subscriber.GetFooInterestingEventCounter(), 0);

  storage.Extend(
      {{kIntConfig, 0}, {kBoolConfig, true}, {kDummyConfig, {1, "bar"}}});
  EXPECT_EQ(subscriber.GetBoolConfigChangeCounter(), 1);
  EXPECT_EQ(subscriber.GetFooInterestingEventCounter(), 0);

  storage.Extend(
      {{kIntConfig, 1}, {kBoolConfig, true}, {kDummyConfig, {0, "bar"}}});
  EXPECT_EQ(subscriber.GetBoolConfigChangeCounter(), 1);
  EXPECT_EQ(subscriber.GetFooInterestingEventCounter(), 1);
}

const dynamic_config::Key<formats::json::Value> kJsonConfig{
    dynamic_config::ConstantConfig{}, {}};

UTEST(DynamicConfig, JsonConfig) {
  // KeyValue automatically parses any config from formats::json::Value. But the
  // config itself can also have the type of formats::json::Value. In this case
  // both approaches are correct. Check that there is no ambiguity.
  const dynamic_config::StorageMock storage{{kJsonConfig, kJson}};
  const auto snapshot = storage.GetSnapshot();
  EXPECT_EQ(snapshot[kJsonConfig], kJson);
}

}  // namespace

USERVER_NAMESPACE_END
