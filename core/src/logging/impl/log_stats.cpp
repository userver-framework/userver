#include <userver/logging/impl/log_stats.hpp>

#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

void DumpMetric(utils::statistics::Writer& writer,
                const impl::LogStatistics& stats) {
  writer["dropped"].ValueWithLabels(stats.dropped, {"version", "2"});

  utils::statistics::Rate total;

  for (size_t i = 0; i < stats.by_level.size(); ++i) {
    const auto by_level = stats.by_level[i].Load();
    writer["by_level"].ValueWithLabels(
        by_level, {"level", ToString(static_cast<Level>(i))});
    total += by_level;
  }

  writer["total"] = total;
  writer["has_reopening_error"] = stats.has_reopening_error.load();
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
