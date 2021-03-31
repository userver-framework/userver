#include <utest/utest.hpp>

#include <taxi_config/config.hpp>
#include <taxi_config/storage_mock.hpp>

namespace {

struct DummyConfig {
  int foo;
  std::string bar;

  static DummyConfig Parse(const taxi_config::DocsMap&) { return {}; }
};

constexpr taxi_config::Key<DummyConfig::Parse> kMyConfig;

int ParseIntConfig(const taxi_config::DocsMap&) { return {}; }

constexpr taxi_config::Key<ParseIntConfig> kIntConfig;

bool ParseBoolConfig(const taxi_config::DocsMap&) { return {}; }

constexpr taxi_config::Key<ParseBoolConfig> kBoolConfig;

class TaxiConfigTest : public testing::Test {
 protected:
  taxi_config::StorageMock storage_{{kMyConfig, {42, "what"}}, {kIntConfig, 5}};
  taxi_config::SnapshotPtr<taxi_config::Config> snapshot_ =
      storage_.GetSnapshot<taxi_config::Config>();
  const taxi_config::Config& config_ = *snapshot_;
};

}  // namespace

TEST_F(TaxiConfigTest, GetExistingConfigClass) {
  const auto& my_config = config_[kMyConfig];
  EXPECT_EQ(my_config.foo, 42);
  EXPECT_EQ(my_config.bar, "what");
}

TEST_F(TaxiConfigTest, GetExistingConfigTrivial) {
  const auto& int_config = config_[kIntConfig];
  EXPECT_EQ(int_config, 5);
}

TEST_F(TaxiConfigTest, GetMissingConfig) {
  EXPECT_THROW((*snapshot_)[kBoolConfig], std::logic_error);
}
