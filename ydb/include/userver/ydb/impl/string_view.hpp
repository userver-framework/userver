#pragma once

#include <library/cpp/monlib/metrics/metric_type.h>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

using StringView = std::invoke_result_t<decltype(&NMonitoring::MetricTypeToStr), NMonitoring::EMetricType>;

}  // namespace ydb::impl

USERVER_NAMESPACE_END
