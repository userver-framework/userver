#include <storages/odbc/detail/deadline.hpp>

#include <algorithm>

#include <userver/server/request/task_inherited_data.hpp>
#include <userver/storages/odbc/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

engine::Deadline MergeWithInheritedDeadline(engine::Deadline operation_deadline) noexcept {
    const auto inherited = server::request::GetTaskInheritedDeadline();
    return std::min(operation_deadline, inherited);
}

void CheckDeadlineNotExpired(const engine::Deadline& deadline) {
    if (!deadline.IsReachable()) {
        return;
    }
    if (deadline.IsReached()) {
        server::request::MarkTaskInheritedDeadlineExpired();
        throw OperationInterrupted("Cancelled by deadline");
    }
}

}  // namespace storages::odbc::detail

USERVER_NAMESPACE_END
