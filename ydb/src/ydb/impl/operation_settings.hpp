#pragma once

#include <chrono>

#include <ydb-cpp-sdk/client/query/query.h>
#include <ydb-cpp-sdk/client/retry/retry.h>
#include <ydb-cpp-sdk/client/table/table.h>
#include <ydb-cpp-sdk/client/types/request_settings.h>

#include <userver/engine/deadline.hpp>
#include <userver/ydb/impl/cast.hpp>

#include <ydb/impl/config.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

constexpr double kOperationTimeoutMultiplier = 0.8;

std::chrono::milliseconds GetBoundTimeout(std::chrono::milliseconds timeout, engine::Deadline deadline);

template <typename T>
void ApplyToRequestSettings(
    NYdb::TRequestSettings<T>& result,
    const OperationSettings& settings,
    engine::Deadline deadline
) {
    result.ClientTimeout(GetBoundTimeout(settings.client_timeout_ms, deadline));

    if (!settings.trace_id.empty()) {
        result.TraceId(impl::ToString(settings.trace_id));
    }
}

template <typename T>
void ApplyToRequestSettings(
    NYdb::TOperationRequestSettings<T>& result,
    const OperationSettings& settings,
    engine::Deadline deadline
) {
    auto timeout = GetBoundTimeout(settings.client_timeout_ms, deadline);

    result.ClientTimeout(timeout);
    result.OperationTimeout(timeout * kOperationTimeoutMultiplier);
    result.CancelAfter(timeout * kOperationTimeoutMultiplier);

    if (!settings.trace_id.empty()) {
        result.TraceId(impl::ToString(settings.trace_id));
    }
}

template <typename T>
T PrepareRequestSettings(const OperationSettings& settings, engine::Deadline deadline) {
    T result;
    impl::ApplyToRequestSettings(result, settings, deadline);
    return result;
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
