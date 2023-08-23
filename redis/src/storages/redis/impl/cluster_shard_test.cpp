#include "cluster_shard.hpp"

#include <tuple>

#include <gtest/gtest.h>

#include <storages/redis/impl/sentinel_impl.hpp>
#include <userver/storages/redis/impl/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
const auto constexpr kNearestServerPing = true;
const auto constexpr kEveryDc = false;
const auto constexpr kDoNotAllowReadFromMaster = false;
const auto constexpr kAllowReadFromMaster = true;
///{ Our test shard has 3 instances: 1 master and 2 replicas
const auto constexpr kServersCount = 3;
const auto constexpr kInstanceReplica0 = 0;
const auto constexpr kInstanceReplica1 = 1;
const auto constexpr kInstanceMaster = 2;
///}

redis::CommandControl MakeCC(bool allow_reads_from_master = false,
                             redis::CommandControl::Strategy strategy =
                                 redis::CommandControl::Strategy::kDefault,
                             size_t best_dc_count = 0) {
  redis::CommandControl ret;
  ret.allow_reads_from_master = allow_reads_from_master;
  ret.best_dc_count = best_dc_count;
  ret.strategy = strategy;
  return ret;
}

}  // namespace

class ClusterShardGetStatIndexTests
    : public ::testing::TestWithParam<
          std::tuple<redis::CommandControl,
                     size_t,  ///< attempt
                     bool,    ///< is_nearest_ping_server
                     size_t,  ///< prev_instance_idx
                     size_t,  ///< current
                     size_t,  ///< servers_count
                     size_t   ///< expected result
                     >> {};

TEST_P(ClusterShardGetStatIndexTests, Base) {
  const auto cc = std::get<0>(GetParam());
  const auto attempt = std::get<1>(GetParam());
  const auto is_nearest_ping_server = std::get<2>(GetParam());
  const auto prev_instance_idx = std::get<3>(GetParam());
  const auto current = std::get<4>(GetParam());
  const auto servers_count = std::get<5>(GetParam());
  const auto expected_result = std::get<6>(GetParam());
  EXPECT_EQ(expected_result,
            redis::GetStartIndex(cc, attempt, is_nearest_ping_server,
                                 prev_instance_idx, current, servers_count));
}

INSTANTIATE_TEST_SUITE_P(
    TestCasses, ClusterShardGetStatIndexTests,
    ::testing::Values(
        ///{ Test read_only request first try, first attempt, every dc,
        ///  do not allow reads from master. So expected results only 0 and 1
        std::make_tuple(MakeCC(), 0, kEveryDc,
                        redis::SentinelImplBase::kDefaultPrevInstanceIdx, 0,
                        kServersCount, kInstanceReplica0),
        std::make_tuple(MakeCC(), 0, kEveryDc,
                        redis::SentinelImplBase::kDefaultPrevInstanceIdx, 1,
                        kServersCount, kInstanceReplica1),
        std::make_tuple(MakeCC(), 0, kEveryDc,
                        redis::SentinelImplBase::kDefaultPrevInstanceIdx, 2,
                        kServersCount, kInstanceReplica0),
        std::make_tuple(MakeCC(), 0, kEveryDc,
                        redis::SentinelImplBase::kDefaultPrevInstanceIdx, 3,
                        kServersCount, kInstanceReplica1),
        ///}

        ///{ Test read_only request first try, second attempt, every dc,
        ///  do not allow reads from master. But now it is possible to read from
        /// master because first attempt failed. So expected results 0, 1 and 2
        std::make_tuple(MakeCC(), 1, kEveryDc,
                        redis::SentinelImplBase::kDefaultPrevInstanceIdx, 0,
                        kServersCount, kInstanceReplica1),
        std::make_tuple(MakeCC(), 1, kEveryDc,
                        redis::SentinelImplBase::kDefaultPrevInstanceIdx, 1,
                        kServersCount, kInstanceMaster),
        std::make_tuple(MakeCC(), 1, kEveryDc,
                        redis::SentinelImplBase::kDefaultPrevInstanceIdx, 2,
                        kServersCount, kInstanceReplica0),
        std::make_tuple(MakeCC(), 1, kEveryDc,
                        redis::SentinelImplBase::kDefaultPrevInstanceIdx, 3,
                        kServersCount, kInstanceReplica1),
        ///}

        ///{ Test read_only request retry (change previous instance idx), first
        ///  attempt, every dc, do not allow reads from master.
        ///  But it is retry so expected results only 0, 1, 2.
        std::make_tuple(MakeCC(), 0, kEveryDc, 0, 1, kServersCount,
                        kInstanceReplica1),
        std::make_tuple(MakeCC(), 0, kEveryDc, 1, 1, kServersCount,
                        kInstanceMaster),
        std::make_tuple(MakeCC(), 0, kEveryDc, 2, 1, kServersCount,
                        kInstanceReplica0),
        ///}

        ///{ Test read_only request first try, first attempt, every dc,
        ///  but allow reads from master. So expected results 0, 1 and 2
        std::make_tuple(MakeCC(true), 0, kEveryDc,
                        redis::SentinelImplBase::kDefaultPrevInstanceIdx, 0,
                        kServersCount, kInstanceReplica0),
        std::make_tuple(MakeCC(true), 0, kEveryDc,
                        redis::SentinelImplBase::kDefaultPrevInstanceIdx, 1,
                        kServersCount, kInstanceReplica1),
        std::make_tuple(MakeCC(true), 0, kEveryDc,
                        redis::SentinelImplBase::kDefaultPrevInstanceIdx, 2,
                        kServersCount, kInstanceMaster),
        std::make_tuple(MakeCC(true), 0, kEveryDc,
                        redis::SentinelImplBase::kDefaultPrevInstanceIdx, 3,
                        kServersCount, kInstanceReplica0),
        ///}

        ///{ Test read_only request first try, first attempt, NearestPingServer,
        ///  do not allow reads from master.
        ///  We always want one of fastest server. the less index the faster.
        ///  But we can cylcle through first best_dc_count servers and default
        ///  value is 0.
        ///  So expected results only 0, 1
        std::make_tuple(MakeCC(), 0, kNearestServerPing,
                        redis::SentinelImplBase::kDefaultPrevInstanceIdx, 0,
                        kServersCount, 0),
        std::make_tuple(MakeCC(), 0, kNearestServerPing,
                        redis::SentinelImplBase::kDefaultPrevInstanceIdx, 1,
                        kServersCount, 1),
        std::make_tuple(MakeCC(), 0, kNearestServerPing,
                        redis::SentinelImplBase::kDefaultPrevInstanceIdx, 2,
                        kServersCount, 0),
        std::make_tuple(MakeCC(), 0, kNearestServerPing,
                        redis::SentinelImplBase::kDefaultPrevInstanceIdx, 3,
                        kServersCount, 1),
        ///}

        ///{ Test read_only request first try, first attempt, NearestPingServer,
        ///  allow reads from master.
        ///  We always want one of fastest server. the less index the faster.
        ///  But we can cylcle through first best_dc_count servers now it is 1.
        ///  So expected results only 0, only the best each time
        std::make_tuple(
            MakeCC(kAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 1),
            0, kNearestServerPing,
            redis::SentinelImplBase::kDefaultPrevInstanceIdx, 0, kServersCount,
            0),
        std::make_tuple(
            MakeCC(kAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 1),
            0, kNearestServerPing,
            redis::SentinelImplBase::kDefaultPrevInstanceIdx, 1, kServersCount,
            0),
        std::make_tuple(
            MakeCC(kAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 1),
            0, kNearestServerPing,
            redis::SentinelImplBase::kDefaultPrevInstanceIdx, 2, kServersCount,
            0),
        std::make_tuple(
            MakeCC(kAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 1),
            0, kNearestServerPing,
            redis::SentinelImplBase::kDefaultPrevInstanceIdx, 3, kServersCount,
            0),
        ///}

        ///{ Test read_only request first try, first attempt, NearestPingServer,
        ///  allow reads from master.
        ///  We always want one of fastest server. the less index the faster.
        ///  But we can cylcle through first best_dc_count servers now it is 2.
        ///  So expected results only 0 and 1, only one of the best each time
        ///  (one of this numbers can be master)
        std::make_tuple(
            MakeCC(kAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 2),
            0, kNearestServerPing,
            redis::SentinelImplBase::kDefaultPrevInstanceIdx, 0, kServersCount,
            0),
        std::make_tuple(
            MakeCC(kAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 2),
            0, kNearestServerPing,
            redis::SentinelImplBase::kDefaultPrevInstanceIdx, 1, kServersCount,
            1),
        std::make_tuple(
            MakeCC(kAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 2),
            0, kNearestServerPing,
            redis::SentinelImplBase::kDefaultPrevInstanceIdx, 2, kServersCount,
            0),
        std::make_tuple(
            MakeCC(kAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 2),
            0, kNearestServerPing,
            redis::SentinelImplBase::kDefaultPrevInstanceIdx, 3, kServersCount,
            1),
        ///}

        ///{ Test read_only request first try, first attempt, NearestPingServer,
        ///  allow reads from master.
        ///  We always want one of fastest server. the less index the faster.
        ///  But we can cylcle through first best_dc_count servers now it is 3.
        ///  So expected results only 0, 1 and 2 (one of this numbers is master)
        std::make_tuple(
            MakeCC(kAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 3),
            0, kNearestServerPing,
            redis::SentinelImplBase::kDefaultPrevInstanceIdx, 0, kServersCount,
            0),
        std::make_tuple(
            MakeCC(kAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 3),
            0, kNearestServerPing,
            redis::SentinelImplBase::kDefaultPrevInstanceIdx, 1, kServersCount,
            1),
        std::make_tuple(
            MakeCC(kAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 3),
            0, kNearestServerPing,
            redis::SentinelImplBase::kDefaultPrevInstanceIdx, 2, kServersCount,
            2),
        std::make_tuple(
            MakeCC(kAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 3),
            0, kNearestServerPing,
            redis::SentinelImplBase::kDefaultPrevInstanceIdx, 3, kServersCount,
            0),
        ///}

        ///{ Test read_only request first try, first attempt, NearestPingServer,
        ///  do not allow reads from master.
        ///  Expected results only 0, 1. For first request we try to choose
        ///  one of the replicas. Master has last position in list of available
        ///  servers for NearestServerPing and not allowed reads from master
        std::make_tuple(
            MakeCC(kDoNotAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 3),
            0, kNearestServerPing,
            redis::SentinelImplBase::kDefaultPrevInstanceIdx, 0, kServersCount,
            0),
        std::make_tuple(
            MakeCC(kDoNotAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 3),
            0, kNearestServerPing,
            redis::SentinelImplBase::kDefaultPrevInstanceIdx, 1, kServersCount,
            1),
        std::make_tuple(
            MakeCC(kDoNotAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 3),
            0, kNearestServerPing,
            redis::SentinelImplBase::kDefaultPrevInstanceIdx, 2, kServersCount,
            0),
        std::make_tuple(
            MakeCC(kDoNotAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 3),
            0, kNearestServerPing,
            redis::SentinelImplBase::kDefaultPrevInstanceIdx, 3, kServersCount,
            1),
        ///}

        std::make_tuple(
            MakeCC(kDoNotAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 3),
            0, kNearestServerPing, 0, 0, kServersCount, 1),
        std::make_tuple(
            MakeCC(kDoNotAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 3),
            0, kNearestServerPing, 1, 1, kServersCount,
            kInstanceMaster),  // Can be master as retry
        std::make_tuple(
            MakeCC(kDoNotAllowReadFromMaster,
                   redis::CommandControl::Strategy::kNearestServerPing, 3),
            0, kNearestServerPing, 2, 2, kServersCount, 0)));

USERVER_NAMESPACE_END
