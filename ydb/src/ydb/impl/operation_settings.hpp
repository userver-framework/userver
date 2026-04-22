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
std::chrono::milliseconds GetBoundTimeout(std::optional<std::chrono::milliseconds> timeout, engine::Deadline deadline);

template <typename T, typename Settings>
void ApplyToRequestSettings(
    NYdb::TRequestSettings<T>& result,
    const Settings& settings,
    engine::Deadline deadline,
    std::string_view trace_id
) {
    result.ClientTimeout(GetBoundTimeout(settings.client_timeout_ms, deadline));

    if (!trace_id.empty()) {
        result.TraceId(impl::ToString(trace_id));
    }
}

template <typename T, typename Settings>
void ApplyToRequestSettings(
    NYdb::TOperationRequestSettings<T>& result,
    const Settings& settings,
    engine::Deadline deadline,
    std::string_view trace_id
) {
    std::chrono::milliseconds timeout;

    if constexpr (std::is_same_v<Settings, OperationSettings> &&
                  (std::is_same_v<T, NYdb::NQuery::TCreateSessionSettings> ||
                   std::is_same_v<T, NYdb::NTable::TCreateSessionSettings>))
    {
        timeout = GetBoundTimeout(settings.get_session_timeout_ms, deadline);
    } else {
        timeout = GetBoundTimeout(settings.client_timeout_ms, deadline);
    }

    result.ClientTimeout(timeout);
    result.OperationTimeout(timeout * kOperationTimeoutMultiplier);
    result.CancelAfter(timeout * kOperationTimeoutMultiplier);

    if (!trace_id.empty()) {
        result.TraceId(impl::ToString(trace_id));
    }
}

template <typename T>
T PrepareRequestSettings(const OperationSettings& settings, engine::Deadline deadline) {
    T result;
    impl::ApplyToRequestSettings(result, settings, deadline, settings.trace_id);
    return result;
}

template <typename T, typename Settings>
T PrepareRequestSettings(const Settings& settings, engine::Deadline deadline, std::string_view trace_id) {
    T result;
    impl::ApplyToRequestSettings(result, settings, deadline, trace_id);
    return result;
}

}  // namespace ydb::impl

USERVER_NAMESPACE_END
