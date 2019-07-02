#include <utest/taxi_config.hpp>

#include <gtest/gtest.h>

#include <cache/cache_config.hpp>
#include <fs/blocking/read.hpp>

namespace utest {
namespace impl {

taxi_config::Config DoReadDefaultTaxiConfig(const std::string& filename) {
  auto contents = fs::blocking::ReadFileContents(filename);

  ::taxi_config::DocsMap docs_map;
  docs_map.Parse(contents, false);

  return taxi_config::Config(docs_map);
}

const taxi_config::Config& ReadDefaultTaxiConfig(const std::string& filename) {
  static const auto config = DoReadDefaultTaxiConfig(filename);
  return config;
}

}  // namespace impl

namespace {

class CacheConfigSetEnvironment : public ::testing::Environment {
 public:
  void SetUp() override {
    if (taxi_config::ConfigModule<taxi_config::FullConfigTag,
                                  cache::CacheConfigSet>::IsRegistered()) {
      LOG_INFO() << "Unregistering cache::CacheConfigSet for tests";
      taxi_config::ConfigModule<taxi_config::FullConfigTag,
                                cache::CacheConfigSet>::Unregister();
    }
  }
};

}  // namespace

::testing::Environment* const foo_env =
    ::testing::AddGlobalTestEnvironment(new CacheConfigSetEnvironment());

}  // namespace utest
