#include <utest/utest.hpp>

#include <taxi_config/config.hpp>
#include <taxi_config/config_ptr.hpp>
#include <taxi_config/storage_mock.hpp>

namespace {

struct DummyConfig final {
  int foo;
  std::string bar;

  static DummyConfig Parse(const taxi_config::DocsMap&) { return {}; }
};

constexpr taxi_config::Key<DummyConfig::Parse> kDummyConfig;

int ParseIntConfig(const taxi_config::DocsMap&) { return {}; }

constexpr taxi_config::Key<ParseIntConfig> kIntConfig;

bool ParseBoolConfig(const taxi_config::DocsMap&) { return {}; }

constexpr taxi_config::Key<ParseBoolConfig> kBoolConfig;

struct DummyConfigWrapper final {
  int GetFoo() const { return config[kDummyConfig].foo; }

  taxi_config::SnapshotPtr config;
};

class TaxiConfigTest : public testing::Test {
 protected:
  const taxi_config::StorageMock storage_{{kDummyConfig, {42, "what"}},
                                          {kIntConfig, 5}};
  const taxi_config::Source source_ = storage_.GetSource();
  const taxi_config::SnapshotPtr snapshot_ = source_.GetSnapshot();
  const taxi_config::Config& config_ = *snapshot_;
};

}  // namespace

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

UTEST_F(TaxiConfigTest, SnapshotPtr) {
  const auto snapshot = source_.GetSnapshot();
  const auto& my_config = snapshot[kDummyConfig];
  EXPECT_EQ(my_config.foo, 42);
  EXPECT_EQ(my_config.bar, "what");
}

UTEST_F(TaxiConfigTest, SnapshotPtrCopyable) {
  DummyConfigWrapper wrapper{snapshot_};  // SnapshotPtr is copied
  EXPECT_EQ(wrapper.GetFoo(), 42);
}

UTEST_F(TaxiConfigTest, VariableSnapshotPtr) {
  const auto my_config = source_.GetSnapshot(kDummyConfig);
  EXPECT_EQ(my_config->foo, 42);
  EXPECT_EQ(my_config->bar, "what");
}

UTEST_F(TaxiConfigTest, Copy) { EXPECT_EQ(source_.GetCopy(kIntConfig), 5); }

namespace {

struct ByConstructor final {
  int foo{42};

  explicit ByConstructor(const taxi_config::DocsMap&) {}

  ByConstructor() = default;
};

}  // namespace

UTEST(TaxiConfig, TheOldWay) {
  // Only for the purposes of testing, don't use in production code
  taxi_config::Key<taxi_config::impl::ParseByConstructor<ByConstructor>> key;
  const taxi_config::StorageMock storage{{key, {}}};

  const auto snapshot = storage.GetSource().GetSnapshot();
  EXPECT_EQ(snapshot->Get<ByConstructor>().foo, 42);
}

namespace {

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

}  // namespace

/// [Sample StorageMock usage]
namespace {

class DummyClient;

std::string DummyFunction(const taxi_config::Config& config) {
  return config[kDummyConfig].bar;
}

}  // namespace

UTEST(TaxiConfig, Snippet) {
  // The 'StorageMock' will only contain the specified configs, and nothing more
  taxi_config::StorageMock storage{
      {kDummyConfig, {42, "what"}},
      {kIntConfig, 5},
  };

  const auto config = storage.GetSource().GetSnapshot();
  EXPECT_EQ(DummyFunction(*config), "what");

  // 'DummyClient' stores 'taxi_config::Source' for access to latest configs
  DummyClient client{storage.GetSource()};
  EXPECT_NO_THROW(client.DoStuff());

  storage.Patch({{kDummyConfig, {-10000, "invalid"}}});
  EXPECT_ANY_THROW(client.DoStuff());
}
/// [Sample StorageMock usage]
