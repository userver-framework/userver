#pragma once

#include <library/cpp/monlib/metrics/metric_registry.h>

#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

void DumpMetric(Writer& writer, const NMonitoring::IMetricSupplier& metric_supplier);

}  // namespace utils::statistics

USERVER_NAMESPACE_END
