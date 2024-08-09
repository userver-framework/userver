#include <ugrpc/impl/rate_and_gauge.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

void DumpMetric(utils::statistics::Writer& writer,
                const AsRateAndGauge& stats) {
  // old format. Currently it is 'rate' as well, as we are
  // in our final stages of fully moving to RateCounter
  // See https://st.yandex-team.ru/TAXICOMMON-6872
  writer = stats.value;
  // new format, with 'rate' support but into different sensor
  // This sensor will be deleted soon.
  writer["v2"] = stats.value;
}

static_assert(utils::statistics::kHasWriterSupport<AsRateAndGauge>);

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
