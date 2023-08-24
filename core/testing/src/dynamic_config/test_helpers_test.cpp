#include <userver/dynamic_config/test_helpers.hpp>

#include <fstream>
#include <string>

#include <gtest/gtest.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

bool ParseRuntimeCfg(const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("USERVER_BAGGAGE_ENABLED").As<bool>();
}
constexpr dynamic_config::Key<ParseRuntimeCfg> kMyConfig{};

}  // namespace

UTEST(DynamicConfig, DefaultConfig) {
  EXPECT_EQ(true, fs::blocking::FileExists(DEFAULT_DYNAMIC_CONFIG_FILENAME));
  const auto& snapshot = dynamic_config::GetDefaultSnapshot();
  EXPECT_EQ(snapshot[kMyConfig], false);
}

USERVER_NAMESPACE_END
