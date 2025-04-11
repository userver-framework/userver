#include <userver/dynamic_config/test_helpers.hpp>

#include <userver/utest/utest.hpp>

#include <dynamic_config/variables/USERVER_BAGGAGE_ENABLED.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(DynamicConfig, DefaultConfig) {
    const auto& snapshot = dynamic_config::GetDefaultSnapshot();
    EXPECT_EQ(snapshot[::dynamic_config::USERVER_BAGGAGE_ENABLED], false);
}

USERVER_NAMESPACE_END
