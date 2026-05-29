#pragma once

#include <algorithm>
#include <chrono>
#include <optional>
#include <string>
#include <string_view>

#include <cassandra.h>

#include <userver/engine/deadline.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/storages/scylla/exception.hpp>
#include <userver/storages/scylla/session_config.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>

#include <storages/scylla/scylla_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::scylla::impl::driver {

inline constexpr std::string_view kDatabaseScyllaType = "scylla";

inline tracing::Span MakeDbSpan(std::string name, std::string_view keyspace, std::string_view table = {}) {
    tracing::Span span(std::move(name));
    span.AddTag(tracing::kDatabaseType, std::string{kDatabaseScyllaType});
    if (!keyspace.empty()) {
        span.AddTag(tracing::kDatabaseInstance, std::string{keyspace});
    }
    if (!table.empty()) {
        span.AddTag(tracing::kDatabaseCollection, std::string{table});
    }
    return span;
}

inline void ApplyDeadlineAndTimeout(CassStatement* statement, std::chrono::milliseconds configured_timeout) {
    if (engine::current_task::ShouldCancel()) {
        throw CancelledException("ScyllaDB operation cancelled: "
        ) << ToString(engine::current_task::CancellationReason());
    }

    auto effective_timeout = configured_timeout;

    const auto inherited_deadline = server::request::GetTaskInheritedDeadline();
    if (inherited_deadline.IsReachable()) {
        const auto left = inherited_deadline.TimeLeftApprox();
        if (left <= std::chrono::seconds{0}) {
            throw CancelledException(CancelledException::ByDeadlinePropagation{});
        }
        const auto left_ms = std::chrono::duration_cast<std::chrono::milliseconds>(left);
        effective_timeout = std::min(effective_timeout, left_ms);
    }

    if (effective_timeout.count() > 0) {
        cass_statement_set_request_timeout(statement, static_cast<cass_uint64_t>(effective_timeout.count()));
    }

    auto* span = tracing::Span::CurrentSpanUnchecked();
    if (span) {
        span->AddTag("db.request_timeout_ms", effective_timeout.count());
    }
}

inline void ApplyConsistency(
    CassStatement* statement,
    const SessionConfig& session_config,
    const std::optional<CommandControl>& cc
) {
    const auto consistency = cc && cc->consistency ? *cc->consistency : session_config.consistency;
    cass_statement_set_consistency(statement, static_cast<CassConsistency>(consistency));

    const auto serial = cc && cc->serial_consistency ? *cc->serial_consistency : session_config.serial_consistency;
    cass_statement_set_serial_consistency(statement, static_cast<CassConsistency>(serial));
}

inline void MarkIdempotent(CassStatement* statement) { cass_statement_set_is_idempotent(statement, cass_true); }

}  // namespace storages::scylla::impl::driver

USERVER_NAMESPACE_END
