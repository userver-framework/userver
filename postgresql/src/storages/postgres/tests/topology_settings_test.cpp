#include <userver/utest/utest.hpp>

#include <storages/postgres/postgres_config.hpp>
#include <userver/formats/yaml/exception.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/yaml_config/yaml_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

TEST(PostgreTopologySettings, RttThresholdDefaultsToTwentyMilliseconds) {
    using namespace std::chrono_literals;

    const pg::TopologySettings settings;

    EXPECT_EQ(settings.rtt_threshold, 20ms);
}

TEST(PostgreTopologySettings, RttThresholdKillSwitchControlsEffectiveThreshold) {
    using namespace std::chrono_literals;

    pg::TopologySettings settings;
    settings.rtt_threshold = 15ms;

    const auto enabled_threshold = settings.GetEffectiveRttThreshold();
    ASSERT_TRUE(enabled_threshold);
    EXPECT_EQ(*enabled_threshold, 15ms);

    settings.rtt_threshold_enabled = false;
    EXPECT_FALSE(settings.GetEffectiveRttThreshold());
}

TEST(PostgreTopologySettings, InvalidStaticRttThresholdIsRejected) {
    const yaml_config::YamlConfig negative_config{formats::yaml::FromString("rtt_threshold: -1ms"), {}};

    UEXPECT_THROW_MSG(negative_config.As<pg::TopologySettings>(), formats::yaml::ParseException, "is negative");

    const yaml_config::YamlConfig too_large_config{formats::yaml::FromString("rtt_threshold: 60001ms"), {}};

    UEXPECT_THROW_MSG(too_large_config.As<pg::TopologySettings>(), pg::InvalidConfig, "must not exceed 60s");
}

USERVER_NAMESPACE_END
