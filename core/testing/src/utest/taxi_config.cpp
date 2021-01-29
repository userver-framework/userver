#include <utest/taxi_config.hpp>

#include <gtest/gtest.h>

#include <cache/cache_config.hpp>
#include <fs/blocking/read.hpp>
#include <logging/log.hpp>

namespace utest {
namespace impl {

taxi_config::Config DoReadDefaultTaxiConfig(const std::string& filename) {
  auto contents = fs::blocking::ReadFileContents(filename);

  ::taxi_config::DocsMap docs_map;
  docs_map.Parse(contents, false);

  return taxi_config::Config(docs_map);
}

std::shared_ptr<const taxi_config::Config> ReadDefaultTaxiConfigPtr(
    const std::string& filename) {
  static const auto config = std::make_shared<const taxi_config::Config>(
      DoReadDefaultTaxiConfig(filename));
  return config;
}

}  // namespace impl

namespace {

class CacheConfigSetEnvironment : public ::testing::Environment {
 public:
  void SetUp() override {
    if (taxi_config::Config::IsRegistered<cache::CacheConfigSet>()) {
      LOG_INFO() << "Unregistering cache::CacheConfigSet for tests";
      taxi_config::Config::Unregister<cache::CacheConfigSet>();
    }
  }
};

}  // namespace

::testing::Environment* const foo_env =
    ::testing::AddGlobalTestEnvironment(new CacheConfigSetEnvironment());

}  // namespace utest
