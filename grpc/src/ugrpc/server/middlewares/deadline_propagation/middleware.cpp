#include <userver/ugrpc/server/middlewares/deadline_propagation/middleware.hpp>

#include <google/protobuf/util/time_util.h>

#include <utility>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utils/impl/internal_tag.hpp>

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

bool CheckAndSetupDeadline(
    tracing::Span& span,
    grpc::ServerContext& server_context,
    std::string_view service_name,
    std::string_view method_name,
    ugrpc::impl::RpcStatisticsScope& statistics_scope,
    const dynamic_config::Snapshot& config,
    utils::AnyStorage<StorageContext>& context,
    bool prefer_absolute_deadline
) {
    if (!config[::dynamic_config::USERVER_DEADLINE_PROPAGATION_ENABLED]) {
        return true;
    }

    std::optional<USERVER_NAMESPACE::server::request::TaskInheritedOriginalDeadline> absolute_original_deadline;
    const auto& client_metadata = server_context.client_metadata();
    const auto absolute_deadline_it = client_metadata.find("x-request-deadline");
    if (absolute_deadline_it != client_metadata.end()) {
        absolute_original_deadline = USERVER_NAMESPACE::server::request::impl::ParseXRequestDeadlineString(
            ugrpc::impl::ToString(absolute_deadline_it->second)
        );
    }

    const auto deadline_duration = ugrpc::TimespecToDuration(server_context.raw_deadline());
    const bool has_grpc_deadline = (deadline_duration != engine::Deadline::Duration::max());
    std::optional<std::chrono::milliseconds> grpc_client_deadline_ms;
    if (has_grpc_deadline) {
        grpc_client_deadline_ms = std::chrono::duration_cast<std::chrono::milliseconds>(deadline_duration);
    }

    auto chosen_deadline = USERVER_NAMESPACE::server::request::impl::TryMakeDeadlinePreferringAbsoluteTimestamp(
        absolute_original_deadline,
        prefer_absolute_deadline,
        grpc_client_deadline_ms,
        config,
        &span
    );

    if (!chosen_deadline.has_value()) {
        if (!has_grpc_deadline) {
            return true;
        }
        chosen_deadline = engine::Deadline::FromDuration(deadline_duration);
    }

    const auto time_left =
        *chosen_deadline == engine::Deadline::Passed()
            ? engine::Deadline::Duration::zero()
            : chosen_deadline->GetTimePoint() - span.GetStartSteadyTime();
    context.Emplace(kDeadlineReceivedKey, time_left);
    span.AddNonInheritableTag(
        "deadline_received_ms",
        std::chrono::duration_cast<std::chrono::milliseconds>(time_left).count()
    );

    statistics_scope.OnDeadlinePropagated();

    USERVER_NAMESPACE::server::request::kTaskInheritedData.Set(USERVER_NAMESPACE::server::request::TaskInheritedData{
        service_name,
        method_name,
        span.GetStartSteadyTime(),
        *chosen_deadline,
        {},
        absolute_original_deadline,
    });

    const bool cancelled_by_deadline = engine::current_task::ShouldCancel() || chosen_deadline->IsSurelyReachedApprox();
    span.AddNonInheritableTag("cancelled_by_deadline", cancelled_by_deadline);
    if (cancelled_by_deadline && config[::dynamic_config::USERVER_GRPC_SERVER_CANCEL_TASK_BY_DEADLINE]) {
        // Experiment and config are enabled
        statistics_scope.OnCancelledByDeadlinePropagation();
        return false;
    }

    return true;
}

}  // namespace

Middleware::Middleware(Settings&& settings)
    : settings_{std::move(settings)}
{}

void Middleware::OnCallStart(MiddlewareCallContext& context) const {
    if (!CheckAndSetupDeadline(
            context.GetSpan(),
            context.GetServerContext(),
            context.GetServiceName(),
            context.GetMethodName(),
            context.GetStatistics(utils::impl::InternalTag{}),
            context.GetInitialDynamicConfig(),
            context.GetStorageContext(),
            settings_.prefer_absolute_deadline
        ))
    {
        context.SetError(grpc::Status{
            grpc::StatusCode::DEADLINE_EXCEEDED,
            "Deadline propagation: Not enough time to handle this call"
        });
        return;
    }
}

void Middleware::PreSendStatus(MiddlewareCallContext& context, grpc::Status& status) const {
    const auto* const inherited_data = USERVER_NAMESPACE::server::request::kTaskInheritedData.GetOptional();

    // if !USERVER_DEADLINE_PROPAGATION_ENABLED, inherited_data must be nullptr
    if (!inherited_data) {
        return;
    }

    if (!inherited_data->deadline.IsReachable()) {
        return;
    }

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
        context.SetError(grpc::Status{
            grpc::StatusCode::DEADLINE_EXCEEDED,
            "Deadline specified by the client for this RPC was exceeded"
        });
        return;
    }
}

}  // namespace ugrpc::server::middlewares::deadline_propagation

USERVER_NAMESPACE_END
