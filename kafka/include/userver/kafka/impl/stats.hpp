#pragma once

#include <userver/rcu/rcu_map.hpp>
#include <userver/utils/statistics/min_max_avg.hpp>
#include <userver/utils/statistics/recentperiod.hpp>
#include <userver/utils/statistics/relaxed_counter.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

using MinMaxAvg = utils::statistics::MinMaxAvg<uint32_t>;

struct MessagesCounts final {
  utils::statistics::RelaxedCounter<uint64_t> messages_total = 0;
  utils::statistics::RelaxedCounter<uint64_t> messages_success = 0;
  utils::statistics::RelaxedCounter<uint64_t> messages_error = 0;
};

struct TopicStats final {
  MessagesCounts messages_counts;
  utils::statistics::RecentPeriod<MinMaxAvg, MinMaxAvg,
                                  utils::datetime::SteadyClock>
      avg_ms_spent_time;
};

struct Stats final {
  rcu::RcuMap<std::string, TopicStats> topics_stats;
  utils::statistics::RelaxedCounter<uint64_t> connections_error = 0;
};

void DumpMetric(utils::statistics::Writer& writer, const Stats& stats);

}  // namespace kafka::impl

USERVER_NAMESPACE_END
