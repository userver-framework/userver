#include <userver/ugrpc/server/middlewares/deadline_propagation/middleware.hpp>

#include <algorithm>

#include <google/protobuf/util/time_util.h>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/server/handlers/impl/deadline_propagation_config.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utils/algo.hpp>

#include <userver/ugrpc/deadline_timepoint.hpp>

#include <ugrpc/impl/internal_tag.hpp>
#include <ugrpc/impl/rpc_metadata.hpp>
#include <ugrpc/server/impl/server_configs.hpp>
#include <userver/ugrpc/impl/to_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::deadline_propagation {

namespace {

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
    const dynamic_config::Snapshot& config
) {
    if (!config[USERVER_NAMESPACE::server::handlers::impl::kDeadlinePropagationEnabled]) {
        return true;
    }

    auto deadline_duration = ugrpc::impl::ExtractDeadlineDuration(server_context.raw_deadline());

    const auto per_attempt_timeout = ExtractPerAttemptTimeout(server_context);
    if (per_attempt_timeout.has_value()) {
        deadline_duration = std::min(deadline_duration, *per_attempt_timeout);
    }

    if (deadline_duration == engine::Deadline::Duration::max()) {
        return true;
    }

    const auto deadline_duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(deadline_duration);

    const bool cancelled_by_deadline =
        server_context.IsCancelled() || deadline_duration_ms <= engine::Deadline::Duration{0};

    span.AddNonInheritableTag("deadline_received_ms", deadline_duration_ms.count());
    statistics_scope.OnDeadlinePropagated();
    span.AddNonInheritableTag("cancelled_by_deadline", cancelled_by_deadline);

    if (cancelled_by_deadline && config[impl::kServerCancelTaskByDeadline]) {
        // Experiment and config are enabled
        statistics_scope.OnCancelledByDeadlinePropagation();
        return false;
    }

    USERVER_NAMESPACE::server::request::TaskInheritedData inherited_data{
        service_name, method_name, std::chrono::steady_clock::now(), engine::Deadline::FromDuration(deadline_duration)};
    USERVER_NAMESPACE::server::request::kTaskInheritedData.Set(inherited_data);

    return true;
}

}  // namespace

void Middleware::Handle(MiddlewareCallContext& context) const {
    auto& call = context.GetCall();

    if (!CheckAndSetupDeadline(
            call.GetSpan(),
            call.GetContext(),
            context.GetCall().GetServiceName(),
            context.GetCall().GetMethodName(),
            call.GetStatistics(ugrpc::impl::InternalTag()),
            context.GetInitialDynamicConfig()
        )) {
        call.FinishWithError(grpc::Status{
            grpc::StatusCode::DEADLINE_EXCEEDED, "Deadline propagation: Not enough time to handle this call"});
        return;
    }

    context.Next();
}

}  // namespace ugrpc::server::middlewares::deadline_propagation

USERVER_NAMESPACE_END
