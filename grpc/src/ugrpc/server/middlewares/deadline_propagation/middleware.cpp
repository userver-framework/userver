#include <userver/ugrpc/server/middlewares/deadline_propagation/middleware.hpp>

#include <algorithm>

#include <google/protobuf/util/time_util.h>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/impl/internal_tag.hpp>

#include <ugrpc/impl/rpc_metadata.hpp>
#include <userver/ugrpc/impl/statistics_scope.hpp>
#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/status_codes.hpp>
#include <userver/ugrpc/time_utils.hpp>

#include <dynamic_config/variables/USERVER_DEADLINE_PROPAGATION_ENABLED.hpp>
#include <dynamic_config/variables/USERVER_GRPC_SERVER_CANCEL_TASK_BY_DEADLINE.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::deadline_propagation {

namespace {

const utils::AnyStorageDataTag<ugrpc::server::StorageContext, engine::Deadline::Duration> kDeadlineReceivedKey;

std::optional<std::chrono::nanoseconds> ExtractPerAttemptTimeout(grpc::ServerContext& server_context) {
    const auto* per_attempt_timeout_header =
        utils::FindOrNullptr(server_context.client_metadata(), ugrpc::impl::kXYaTaxiPerAttemptTimeout);
    if (per_attempt_timeout_header) {
        google::protobuf::Duration per_attempt_timeout{};
        const bool result = google::protobuf::util::TimeUtil::FromString(
            ugrpc::impl::ToString(*per_attempt_timeout_header), &per_attempt_timeout
        );
        if (result) {
            return std::chrono::nanoseconds{
                google::protobuf::util::TimeUtil::DurationToNanoseconds(per_attempt_timeout)};
        }
    }
    return std::nullopt;
}

bool CheckAndSetupDeadline(
    tracing::Span& span,
    grpc::ServerContext& server_context,
    std::string_view service_name,
    std::string_view method_name,
    ugrpc::impl::RpcStatisticsScope& statistics_scope,
    const dynamic_config::Snapshot& config,
    utils::AnyStorage<StorageContext>& context

) {
    if (!config[::dynamic_config::USERVER_DEADLINE_PROPAGATION_ENABLED]) {
        return true;
    }

    auto deadline_duration = ugrpc::TimespecToDuration(server_context.raw_deadline());

    const auto per_attempt_timeout = ExtractPerAttemptTimeout(server_context);
    if (per_attempt_timeout.has_value()) {
        deadline_duration = std::min(deadline_duration, *per_attempt_timeout);
    }

    if (deadline_duration == engine::Deadline::Duration::max()) {
        return true;
    }

    context.Emplace(kDeadlineReceivedKey, deadline_duration);

    const auto deadline_duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(deadline_duration);

    const bool cancelled_by_deadline =
        server_context.IsCancelled() || deadline_duration_ms <= engine::Deadline::Duration{0};

    span.AddNonInheritableTag("deadline_received_ms", deadline_duration_ms.count());
    statistics_scope.OnDeadlinePropagated();
    span.AddNonInheritableTag("cancelled_by_deadline", cancelled_by_deadline);

    if (cancelled_by_deadline && config[::dynamic_config::USERVER_GRPC_SERVER_CANCEL_TASK_BY_DEADLINE]) {
        // Experiment and config are enabled
        statistics_scope.OnCancelledByDeadlinePropagation();
        return false;
    }

    const USERVER_NAMESPACE::server::request::TaskInheritedData inherited_data{
        service_name, method_name, std::chrono::steady_clock::now(), engine::Deadline::FromDuration(deadline_duration)};
    USERVER_NAMESPACE::server::request::kTaskInheritedData.Set(inherited_data);

    return true;
}

}  // namespace

void Middleware::OnCallStart(MiddlewareCallContext& context) const {
    if (!CheckAndSetupDeadline(
            context.GetSpan(),
            context.GetServerContext(),
            context.GetServiceName(),
            context.GetMethodName(),
            context.GetStatistics(utils::impl::InternalTag{}),
            context.GetInitialDynamicConfig(),
            context.GetStorageContext()
        )) {
        return context.SetError(grpc::Status{
            grpc::StatusCode::DEADLINE_EXCEEDED, "Deadline propagation: Not enough time to handle this call"});
    }
}

void Middleware::OnCallFinish(MiddlewareCallContext& context, const grpc::Status& status) const {
    const auto* const inherited_data = USERVER_NAMESPACE::server::request::kTaskInheritedData.GetOptional();

    // if !USERVER_DEADLINE_PROPAGATION_ENABLED, inherited_data must be nullptr
    if (!inherited_data) return;

    if (!inherited_data->deadline.IsReachable()) return;

    const bool cancelled_by_deadline =
        engine::current_task::CancellationReason() == engine::TaskCancellationReason::kDeadline ||
        inherited_data->deadline_signal.IsExpired() || inherited_data->deadline.IsReached();

    context.GetSpan().AddNonInheritableTag("cancelled_by_deadline", cancelled_by_deadline);

    if (cancelled_by_deadline && status.error_code() != grpc::StatusCode::DEADLINE_EXCEEDED) {
        const auto deadline = context.GetStorageContext().Get(kDeadlineReceivedKey);
        LOG_INFO() << fmt::format(
            "Deadline {}ms specified by the upstream client for this RPC was exceeded. The original status is "
            "replaced "
            "with DEADLINE_EXCEEDED by `grpc-server-deadline-propagation` middleware. Original status: {}, msg: '{}'",
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline).count(),
            ugrpc::ToString(status.error_code()),
            status.error_message()
        );
        return context.SetError(grpc::Status{
            grpc::StatusCode::DEADLINE_EXCEEDED, "Deadline specified by the client for this RPC was exceeded"});
    }
}

}  // namespace ugrpc::server::middlewares::deadline_propagation

USERVER_NAMESPACE_END
