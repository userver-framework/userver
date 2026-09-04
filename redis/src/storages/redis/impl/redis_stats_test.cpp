#include <storages/redis/impl/redis_stats.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {
namespace {

TEST(RedisStats, MsetexCommandTimingsAreRegistered) {
    Statistics statistics;

    EXPECT_NE(statistics.command_timings_percentile.find("msetex"), statistics.command_timings_percentile.end());
}

}  // namespace
}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
