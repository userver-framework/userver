#pragma once

#include <userver/server/request/task_inherited_data.hpp>
#include <userver/ugrpc/client/impl/call_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

inline bool IsTaskCancelledByDeadlinePropagation() noexcept {
    UASSERT(engine::current_task::ShouldCancel());
    return USERVER_NAMESPACE::server::request::GetTaskInheritedDeadline().IsReached();
}

inline bool IsRequestCancelledByDeadlinePropagation(const grpc::Status& status, const CallState& state) noexcept {
    return grpc::StatusCode::DEADLINE_EXCEEDED == status.error_code() && state.IsDeadlinePropagated();
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
