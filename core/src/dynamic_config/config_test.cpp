#include <userver/utest/utest.hpp>

#include <vector>

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

namespace {

class Subscriber final {
 public:
  void OnConfigUpdate(const dynamic_config::Snapshot&) { counter_++; }
  int GetCounter() const { return counter_; }

 private:
  int counter_{};
};

}  // namespace

[[maybe_unused]] bool operator==(const DummyConfig& lhs,
                                 const DummyConfig& rhs) {
  return lhs.foo == rhs.foo && lhs.bar == rhs.bar;
}

UTEST(DynamicConfig, SingleSubscription) {
  std::vector<dynamic_config::KeyValue> vars1{
      {kDummyConfig, {0, "bar"}}, {kIntConfig, 1}, {kBoolConfig, false}};
  std::vector<dynamic_config::KeyValue> vars2{
      {kDummyConfig, {0, "foo"}}, {kIntConfig, 2}, {kBoolConfig, false}};
  std::vector<dynamic_config::KeyValue> vars3{
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
  std::vector<dynamic_config::KeyValue> vars1{{kIntConfig, 1},
                                              {kBoolConfig, false}};
  std::vector<dynamic_config::KeyValue> vars2{{kIntConfig, 1},
                                              {kBoolConfig, true}};
  std::vector<dynamic_config::KeyValue> vars3{{kIntConfig, 2},
                                              {kBoolConfig, true}};

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
  std::vector<dynamic_config::KeyValue> vars1{
      {kDummyConfig, {0, "bar"}}, {kIntConfig, 1}, {kBoolConfig, false}};
  std::vector<dynamic_config::KeyValue> vars2{
      {kDummyConfig, {0, "foo"}}, {kIntConfig, 1}, {kBoolConfig, true}};
  std::vector<dynamic_config::KeyValue> vars3{
      {kDummyConfig, {0, "foo"}}, {kIntConfig, 0}, {kBoolConfig, true}};

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

namespace {

class CustomSubscriber final {
 public:
  void OnConfigUpdate(const dynamic_config::Diff&) { counter_++; }
  int GetCounter() const { return counter_; }

 private:
  int counter_{};
};

}  // namespace

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

namespace {

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

}  // namespace

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

}  // namespace

USERVER_NAMESPACE_END
