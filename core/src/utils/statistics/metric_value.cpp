#include <userver/utils/statistics/metric_value.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

static_assert(std::is_trivially_copyable_v<MetricValue> &&
                  sizeof(MetricValue) <= sizeof(void*) * 2,
              "MetricValue should fit in registers, because it is expected "
              "to be passed around by value");

}  // namespace utils::statistics

USERVER_NAMESPACE_END
