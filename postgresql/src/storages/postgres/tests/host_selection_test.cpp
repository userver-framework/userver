#include <userver/utest/utest.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

#include <storages/postgres/detail/host_selection.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/options.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

TEST(PostgreTopology, NoMaxRttDisablesPreference) {
    namespace topology = storages::postgres::detail::topology;
    using namespace std::chrono_literals;

    const std::vector<pg::detail::Rtt> roundtrip_times{1ms, 100ms};
    topology::TopologyBase::DsnIndices dsn_indices;
    dsn_indices.indices = {0, 1};

    pg::detail::FillDsnIndices(dsn_indices, roundtrip_times, std::nullopt);

    EXPECT_TRUE(dsn_indices.acceptable_indices.empty());
    EXPECT_EQ(dsn_indices.GetRoundRobinIndices(), dsn_indices.indices);
    EXPECT_EQ(dsn_indices.nearest, 0);
}

TEST(PostgreTopology, RttThresholdIsRelativeAndInclusive) {
    namespace topology = storages::postgres::detail::topology;
    using namespace std::chrono_literals;

    const std::vector<pg::detail::Rtt> roundtrip_times{10ms, 29999us, 30ms, 30001us};
    topology::TopologyBase::DsnIndices dsn_indices;
    dsn_indices.indices = {0, 1, 2, 3};

    pg::detail::FillDsnIndices(dsn_indices, roundtrip_times, 10ms + 20ms);

    EXPECT_EQ(dsn_indices.acceptable_indices, (std::vector<topology::TopologyBase::DsnIndex>{0, 1, 2}));

    const std::vector<pg::detail::Rtt> zero_threshold_roundtrip_times{1ms, 1ms, 1001us};
    topology::TopologyBase::DsnIndices zero_dsn_indices;
    zero_dsn_indices.indices = {0, 1, 2};

    pg::detail::FillDsnIndices(zero_dsn_indices, zero_threshold_roundtrip_times, 1ms + 0ms);

    EXPECT_EQ(zero_dsn_indices.acceptable_indices, (std::vector<topology::TopologyBase::DsnIndex>{0, 1}));
}

TEST(PostgreTopology, RttCandidateExtractionPreservesHostOrder) {
    namespace topology = storages::postgres::detail::topology;
    using DsnIndices = topology::TopologyBase::DsnIndices;
    using namespace std::chrono_literals;

    const std::vector<pg::detail::Rtt> roundtrip_times{2ms, 1ms, 500us};
    DsnIndices dsn_indices;
    dsn_indices.indices = {2, 0, 1};

    pg::detail::FillDsnIndices(dsn_indices, roundtrip_times, 500us + 1ms);

    EXPECT_EQ(dsn_indices.indices, (std::vector<topology::TopologyBase::DsnIndex>{2, 0, 1}));
    EXPECT_EQ(dsn_indices.acceptable_indices, (std::vector<topology::TopologyBase::DsnIndex>{2, 1}));
    EXPECT_EQ(dsn_indices.nearest, 2);
}

TEST(PostgreTopology, UnknownRttIsNotPreferred) {
    namespace topology = storages::postgres::detail::topology;
    using DsnIndices = topology::TopologyBase::DsnIndices;
    using namespace std::chrono_literals;

    const std::vector<pg::detail::Rtt> roundtrip_times{pg::detail::kUnknownRtt, 2ms};
    DsnIndices dsn_indices;
    dsn_indices.indices = {0, 1};

    pg::detail::FillDsnIndices(dsn_indices, roundtrip_times, 2ms + 1ms);

    EXPECT_EQ(dsn_indices.acceptable_indices, (std::vector<topology::TopologyBase::DsnIndex>{1}));
    EXPECT_EQ(dsn_indices.GetRoundRobinIndices(), dsn_indices.acceptable_indices);
}

TEST(PostgreTopology, AllUnknownRttUsesAllAliveFallback) {
    namespace topology = storages::postgres::detail::topology;
    using DsnIndices = topology::TopologyBase::DsnIndices;

    const std::vector<pg::detail::Rtt> roundtrip_times{pg::detail::kUnknownRtt, pg::detail::kUnknownRtt};
    DsnIndices dsn_indices;
    dsn_indices.indices = {0, 1};

    pg::detail::FillDsnIndices(dsn_indices, roundtrip_times, pg::detail::Rtt::max());

    EXPECT_TRUE(dsn_indices.acceptable_indices.empty());
    EXPECT_EQ(dsn_indices.GetRoundRobinIndices(), dsn_indices.indices);
}

TEST(PostgreTopology, FillAndRoundRobinPreferRttAcceptableHosts) {
    using TopologyBase = storages::postgres::detail::topology::TopologyBase;
    using namespace std::chrono_literals;

    const std::vector<pg::detail::Rtt> roundtrip_times{10ms, 100ms, 20ms};
    TopologyBase::DsnIndices dsn_indices;
    dsn_indices.indices = {0, 1, 2};

    const pg::TopologySettings topology_settings;
    const auto rtt_threshold = topology_settings.GetEffectiveRttThreshold();
    ASSERT_TRUE(rtt_threshold);
    pg::detail::FillDsnIndices(dsn_indices, roundtrip_times, roundtrip_times.front() + *rtt_threshold);

    EXPECT_EQ(dsn_indices.GetRoundRobinIndices(), (std::vector<TopologyBase::DsnIndex>{0, 2}));
    EXPECT_EQ(dsn_indices.indices, (std::vector<TopologyBase::DsnIndex>{0, 1, 2}));

    std::atomic<std::uint32_t> rr_index{0};
    const pg::ClusterHostTypeFlags flags{pg::ClusterHostType::kRoundRobin};
    EXPECT_EQ(pg::detail::SelectDsnIndex(dsn_indices, flags, rr_index), 0);
    EXPECT_EQ(pg::detail::SelectDsnIndex(dsn_indices, flags, rr_index), 2);
    EXPECT_EQ(pg::detail::SelectDsnIndex(dsn_indices, flags, rr_index), 0);
    EXPECT_EQ(rr_index.load(std::memory_order_relaxed), 3);
}

TEST(PostgreTopology, RttThresholdKillSwitchDisablesRoundRobinPreference) {
    using TopologyBase = storages::postgres::detail::topology::TopologyBase;
    using DsnIndices = TopologyBase::DsnIndices;
    using namespace std::chrono_literals;

    const std::vector<pg::detail::Rtt> roundtrip_times{10ms, 20ms, 100ms};
    DsnIndices dsn_indices;
    dsn_indices.indices = {0, 1, 2};

    pg::TopologySettings topology_settings;
    const auto rtt_threshold = topology_settings.GetEffectiveRttThreshold();
    ASSERT_TRUE(rtt_threshold);
    pg::detail::FillDsnIndices(dsn_indices, roundtrip_times, roundtrip_times.front() + *rtt_threshold);

    EXPECT_EQ(dsn_indices.GetRoundRobinIndices(), (std::vector<TopologyBase::DsnIndex>{0, 1}));

    std::atomic<std::uint32_t> enabled_rr_index{0};
    const pg::ClusterHostTypeFlags flags{pg::ClusterHostType::kRoundRobin};
    EXPECT_EQ(pg::detail::SelectDsnIndex(dsn_indices, flags, enabled_rr_index), 0);
    EXPECT_EQ(pg::detail::SelectDsnIndex(dsn_indices, flags, enabled_rr_index), 1);

    topology_settings.rtt_threshold_enabled = false;
    pg::detail::FillDsnIndices(dsn_indices, roundtrip_times, topology_settings.GetEffectiveRttThreshold());

    EXPECT_EQ(dsn_indices.GetRoundRobinIndices(), (std::vector<TopologyBase::DsnIndex>{0, 1, 2}));

    std::atomic<std::uint32_t> disabled_rr_index{0};
    EXPECT_EQ(pg::detail::SelectDsnIndex(dsn_indices, flags, disabled_rr_index), 0);
    EXPECT_EQ(pg::detail::SelectDsnIndex(dsn_indices, flags, disabled_rr_index), 1);
    EXPECT_EQ(pg::detail::SelectDsnIndex(dsn_indices, flags, disabled_rr_index), 2);
}

TEST(PostgreTopology, SlowSlaveIsUsedBeforeMasterRoleFallback) {
    using TopologyBase = pg::detail::topology::TopologyBase;
    using namespace std::chrono_literals;

    const std::vector<pg::detail::Rtt> roundtrip_times{10ms, 100ms};
    TopologyBase::DsnIndices master_indices;
    master_indices.indices = {0};
    pg::detail::FillDsnIndices(master_indices, roundtrip_times, 30ms);

    TopologyBase::DsnIndices slave_indices;
    slave_indices.indices = {1};
    pg::detail::FillDsnIndices(slave_indices, roundtrip_times, 30ms);

    TopologyBase::DsnIndicesByType indices_by_type{
        {pg::ClusterHostType::kMaster, master_indices},
        {pg::ClusterHostType::kSlave, slave_indices},
    };

    const auto dsn_indices_it = pg::detail::ResolveHostRole(indices_by_type, pg::ClusterHostType::kSlave);
    EXPECT_EQ(dsn_indices_it->first, pg::ClusterHostType::kSlave);

    std::atomic<std::uint32_t> rr_index{0};
    const pg::ClusterHostTypeFlags flags{pg::ClusterHostType::kSlave, pg::ClusterHostType::kRoundRobin};
    EXPECT_EQ(pg::detail::SelectDsnIndex(dsn_indices_it->second, flags, rr_index), 1);

    indices_by_type.erase(pg::ClusterHostType::kSlave);
    const auto fallback_indices_it = pg::detail::ResolveHostRole(indices_by_type, pg::ClusterHostType::kSlave);
    EXPECT_EQ(fallback_indices_it->first, pg::ClusterHostType::kMaster);
    EXPECT_EQ(pg::detail::SelectDsnIndex(fallback_indices_it->second, flags, rr_index), 0);
}

TEST(PostgreTopology, CombinedMasterSlaveUsesAcceptableAliveHost) {
    using TopologyBase = pg::detail::topology::TopologyBase;
    using namespace std::chrono_literals;

    const std::vector<pg::detail::Rtt> roundtrip_times{100ms, 10ms};
    TopologyBase::DsnIndices alive_indices;
    alive_indices.indices = {0, 1};
    pg::detail::FillDsnIndices(alive_indices, roundtrip_times, 30ms);

    std::atomic<std::uint32_t> rr_index{0};
    const pg::ClusterHostTypeFlags flags{
        pg::ClusterHostType::kSlaveOrMaster,
        pg::ClusterHostType::kRoundRobin,
    };
    EXPECT_EQ(pg::detail::SelectDsnIndex(alive_indices, flags, rr_index), 1);
}

TEST(PostgreTopology, StandaloneUsesAllAliveFallback) {
    using TopologyBase = pg::detail::topology::TopologyBase;

    TopologyBase::DsnIndices standalone_indices;
    standalone_indices.indices = {0};
    standalone_indices.nearest = 0;

    std::atomic<std::uint32_t> rr_index{0};
    const pg::ClusterHostTypeFlags flags{pg::ClusterHostType::kMaster, pg::ClusterHostType::kRoundRobin};
    EXPECT_EQ(pg::detail::SelectDsnIndex(standalone_indices, flags, rr_index), 0);
    EXPECT_EQ(rr_index.load(std::memory_order_relaxed), 0);
}

TEST(PostgreTopology, MissingRequestedAndFallbackRolesThrowsClusterUnavailable) {
    const pg::detail::topology::TopologyBase::DsnIndicesByType empty_indices;

    UEXPECT_THROW_MSG(
        pg::detail::ResolveHostRole(empty_indices, pg::ClusterHostType::kSlave),
        pg::ClusterUnavailable,
        "Pool for master (requested: slave) is not available"
    );
}

TEST(PostgreTopology, NearestIgnoresRttAcceptableHosts) {
    using TopologyBase = pg::detail::topology::TopologyBase;

    TopologyBase::DsnIndices dsn_indices;
    dsn_indices.indices = {0, 1, 2};
    dsn_indices.nearest = 1;
    dsn_indices.acceptable_indices = {0, 2};

    std::atomic<std::uint32_t> rr_index{0};
    const pg::ClusterHostTypeFlags flags{pg::ClusterHostType::kNearest};
    EXPECT_EQ(pg::detail::SelectDsnIndex(dsn_indices, flags, rr_index), 1);
    EXPECT_EQ(rr_index.load(std::memory_order_relaxed), 0);
}

USERVER_NAMESPACE_END
