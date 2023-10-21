#include <userver/dynamic_config/test_helpers.hpp>

#include <userver/baggage/baggage_settings.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(DynamicConfig, DefaultConfig) {
  EXPECT_EQ(true, fs::blocking::FileExists(DEFAULT_DYNAMIC_CONFIG_FILENAME));
  const auto& snapshot = dynamic_config::GetDefaultSnapshot();
  EXPECT_EQ(snapshot[baggage::kBaggageEnabled], false);
}

USERVER_NAMESPACE_END
