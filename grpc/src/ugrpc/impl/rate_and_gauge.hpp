#pragma once

#include <userver/utils/statistics/rate.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

// For now, we need to dump metrics in legacy format - without 'rate'
// support - in order not to break existing dashboards
// See https://st.yandex-team.ru/TAXICOMMON-6872
struct AsRateAndGauge final {
  utils::statistics::Rate value;
};

void DumpMetric(utils::statistics::Writer& writer, const AsRateAndGauge& stats);

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
