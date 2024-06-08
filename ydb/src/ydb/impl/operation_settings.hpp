#pragma once

#include <ydb-cpp-sdk/client/retry/retry.h>
#include <ydb-cpp-sdk/client/types/request_settings.h>

#include <algorithm>  // for std::min
#include <chrono>

#include <userver/engine/deadline.hpp>
#include <userver/ydb/impl/cast.hpp>

#include <ydb/impl/config.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

std::chrono::milliseconds DeadlineToTimeout(engine::Deadline deadline);

template <typename T>
void ApplyToRequestSettings(NYdb::TRequestSettings<T>& result,
                            const OperationSettings& settings,
                            engine::Deadline deadline) {
  const auto timeout = impl::DeadlineToTimeout(deadline);

  if (settings.client_timeout_ms > std::chrono::milliseconds::zero()) {
    // That's not the most optimal way to propagate deadline, but oh well.
    result.ClientTimeout(std::min(settings.client_timeout_ms, timeout));
  }
  if (!settings.trace_id.empty()) {
    result.TraceId(impl::ToString(settings.trace_id));
  }
}

template <typename T>
void ApplyToRequestSettings(NYdb::TOperationRequestSettings<T>& result,
                            const OperationSettings& settings,
                            engine::Deadline deadline) {
  const auto timeout = impl::DeadlineToTimeout(deadline);

  if (settings.operation_timeout_ms > std::chrono::milliseconds::zero()) {
    result.OperationTimeout(std::min(settings.operation_timeout_ms, timeout));
  }
  if (settings.cancel_after_ms > std::chrono::milliseconds::zero()) {
    result.CancelAfter(settings.cancel_after_ms);
  }
  if (settings.client_timeout_ms > std::chrono::milliseconds::zero()) {
    result.ClientTimeout(settings.client_timeout_ms);
  }
  if (!settings.trace_id.empty()) {
    result.TraceId(impl::ToString(settings.trace_id));
  }
}

template <typename T>
T PrepareRequestSettings(const OperationSettings& settings,
                         engine::Deadline deadline) {
  T result;
  impl::ApplyToRequestSettings(result, settings, deadline);
  return result;
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
