#include <userver/utils/statistics/striped_rate_counter.hpp>

#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

void DumpMetric(Writer& writer, const StripedRateCounter& value) {
  writer = Rate{value.Load()};
}

void ResetMetric(StripedRateCounter& value) { value.Store(Rate{}); }

}  // namespace utils::statistics

USERVER_NAMESPACE_END
