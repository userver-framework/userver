#include <storages/odbc/detail/deadline.hpp>

#include <userver/server/request/task_inherited_data.hpp>
#include <userver/storages/odbc/exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::detail {

namespace {

engine::Deadline MinReachableDeadline(engine::Deadline a, engine::Deadline b) noexcept {
    if (!a.IsReachable()) {
        return b;
    }
    if (!b.IsReachable()) {
        return a;
    }
    return a < b ? a : b;
}

}  // namespace

engine::Deadline MergeWithInheritedDeadline(engine::Deadline operation_deadline) noexcept {
    const auto inherited = server::request::GetTaskInheritedDeadline();
    return MinReachableDeadline(operation_deadline, inherited);
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
