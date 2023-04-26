#include "log_stats.hpp"

#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::statistics {

void DumpMetric(utils::statistics::Writer& writer, const LogStatistics& stats) {
  writer["dropped"] = stats.dropped;

  Counter total = 0;

  for (size_t i = 0; i < stats.by_level.size(); ++i) {
    writer["by_level"].ValueWithLabels(
        stats.by_level[i], {"level", ToString(static_cast<Level>(i))});
    total += stats.by_level[i];
  }

  writer["total"] = total;
}

}  // namespace logging::statistics

USERVER_NAMESPACE_END
