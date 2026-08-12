#include <userver/server/request/task_inherited_data.hpp>
#include <userver/storages/odbc.hpp>
#include <userver/storages/odbc/exception.hpp>
#include <userver/storages/odbc/tests/utils.hpp>
#include <userver/utest/utest.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests {

namespace {

server::request::TaskInheritedData MakeRequestData(engine::Deadline deadline) {
    return {
        .path = {},
        .method = "GET",
        .start_time = {},
        .deadline = std::move(deadline),
    };
}

}  // namespace

UTEST(OdbcDeadline, CancelledByInheritedDeadlineOnDefaultExecute) {
    auto cluster = MakeCluster();
    server::request::kTaskInheritedData.Set(MakeRequestData(engine::Deadline::FromDuration(-1s)));

    UEXPECT_THROW(
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1"),
        storages::odbc::OperationInterrupted
    );
}

UTEST(OdbcDeadline, InheritedExpiredOverridesLongExplicitExecute) {
    auto cluster = MakeCluster();
    server::request::kTaskInheritedData.Set(MakeRequestData(engine::Deadline::Passed()));

    UEXPECT_THROW(
        cluster.Execute(engine::Deadline::FromDuration(1h), storages::odbc::ClusterHostType::kMaster, "SELECT 1"),
        storages::odbc::OperationInterrupted
    );
}

UTEST(OdbcDeadline, ExplicitExpiredExecute) {
    auto cluster = MakeCluster();

    UEXPECT_THROW(
        cluster.Execute(engine::Deadline::Passed(), storages::odbc::ClusterHostType::kMaster, "SELECT 1"),
        storages::odbc::OperationInterrupted
    );
}

UTEST(OdbcDeadline, CancelledByInheritedDeadlineOnBegin) {
    auto cluster = MakeCluster();
    server::request::kTaskInheritedData.Set(MakeRequestData(engine::Deadline::FromDuration(-1s)));

    UEXPECT_THROW(cluster.Begin(storages::odbc::ClusterHostType::kMaster), storages::odbc::OperationInterrupted);
}

UTEST(OdbcDeadline, DeadlinePropagationBlockerIgnoresInheritedExpiry) {
    auto cluster = MakeCluster();
    server::request::kTaskInheritedData.Set(MakeRequestData(engine::Deadline::Passed()));

    server::request::DeadlinePropagationBlocker blocker;
    UEXPECT_NO_THROW(cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1"));
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END
