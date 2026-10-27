#include "test_utils.hpp"

#include <optional>
#include <utility>

#include <userver/engine/deadline.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
#include <userver/ydb/exceptions.hpp>
#include <userver/ydb/table.hpp>
#include <userver/ydb/transaction.hpp>

USERVER_NAMESPACE_BEGIN

using YdbTransaction = ydb::ClientFixtureBase;

UTEST_F(YdbTransaction, MovedFromDestructorDoesNotRollback) {
    std::optional<ydb::Transaction> moved;
    {
        auto source = GetTableClient().Begin("move");
        moved.emplace(std::move(source));
    }
    EXPECT_FALSE(GetMetrics().SingleMetricOptional("ydb.by-query.total", {{"ydb_query", "Rollback"}}));
    EXPECT_NO_THROW(moved->Execute(ydb::Query{"SELECT 1"}));
    moved.reset();
    EXPECT_EQ(GetMetrics().SingleMetric("ydb.by-query.success", {{"ydb_query", "Rollback"}}).AsRate().value, 1);
}

UTEST_F(YdbTransaction, DestructorRollsBackAfterCancellation) {
    auto task = utils::Async("cancel-transaction", [&] {
        auto transaction = GetTableClient().Begin("cancel");
        engine::current_task::RequestCancel();
    });
    task.Get();
    EXPECT_EQ(GetMetrics().SingleMetric("ydb.by-query.success", {{"ydb_query", "Rollback"}}).AsRate().value, 1);
    const auto metrics = GetMetrics();
    const auto cancelled = metrics.SingleMetric("ydb.by-transaction.cancelled", {{"ydb_transaction", "cancel"}});
    EXPECT_EQ(cancelled.AsRate().value, 1);
}

UTEST_F(YdbTransaction, RollbackAfterInheritedDeadline) {
    utils::impl::UserverExperimentsScope experiments;
    experiments.Set(utils::impl::kYdbDeadlinePropagationExperiment, true);
    auto transaction = GetTableClient().Begin("deadline");
    server::request::kTaskInheritedData.Set(server::request::TaskInheritedData{.deadline = engine::Deadline::Passed()});
    EXPECT_THROW(transaction.Execute(ydb::Query{"SELECT 1"}), ydb::DeadlineExceededError);
    EXPECT_NO_THROW(transaction.Rollback());
    EXPECT_TRUE(server::request::GetTaskInheritedDeadline().IsReached());
    EXPECT_EQ(GetMetrics().SingleMetric("ydb.by-query.success", {{"ydb_query", "Rollback"}}).AsRate().value, 1);
}

UTEST_F(YdbTransaction, CommittedDestructorDoesNotRollback) {
    {
        auto transaction = GetTableClient().Begin("commit");
        transaction.Execute(ydb::Query{"SELECT 1"});
        transaction.Commit();
    }
    EXPECT_FALSE(GetMetrics().SingleMetricOptional("ydb.by-query.total", {{"ydb_query", "Rollback"}}));
}

USERVER_NAMESPACE_END
