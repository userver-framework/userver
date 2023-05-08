#include <userver/utils/statistics/rate_counter.hpp>

#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

void DumpMetric(Writer& writer, const RateCounter& value) {
  writer = Rate{value.Load()};
}

void ResetMetric(RateCounter& value) { value.Store(Rate{0}); }

}  // namespace utils::statistics

USERVER_NAMESPACE_END
