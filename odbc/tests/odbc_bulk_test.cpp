#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <sqlext.h>

#include <storages/odbc/detail/bulk.hpp>
#include <userver/storages/odbc.hpp>
#include <userver/storages/odbc/tests/utils.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/testing.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests {

namespace {

using namespace std::chrono_literals;

static_assert(!std::is_copy_constructible_v<BulkParameterStore>);
static_assert(!std::is_copy_assignable_v<BulkParameterStore>);
static_assert(std::is_nothrow_move_constructible_v<BulkParameterStore>);
static_assert(std::is_nothrow_move_assignable_v<BulkParameterStore>);
static_assert(sizeof(Cluster) == 8);
static_assert(sizeof(Transaction) == 104);

using ClusterBulkExecute =
    BulkResult (Cluster::*)(ClusterHostTypeFlags, const Query&, const BulkParameterStore&, std::size_t);
using ClusterBulkExecuteWithCommandControl = BulkResult (Cluster::*)(
    ClusterHostTypeFlags,
    OptionalCommandControl,
    const Query&,
    const BulkParameterStore&,
    std::size_t
);
using TransactionBulkExecute = BulkResult (Transaction::*)(const Query&, const BulkParameterStore&, std::size_t);
using TransactionBulkExecuteWithCommandControl =
    BulkResult (Transaction::*)(OptionalCommandControl, const Query&, const BulkParameterStore&, std::size_t);

static_assert(requires {
    static_cast<ClusterBulkExecute>(&Cluster::ExecuteBulk);
    static_cast<ClusterBulkExecuteWithCommandControl>(&Cluster::ExecuteBulk);
    static_cast<TransactionBulkExecute>(&Transaction::ExecuteBulk);
    static_cast<TransactionBulkExecuteWithCommandControl>(&Transaction::ExecuteBulk);
});

impl::ParameterRows MakeRows(std::initializer_list<impl::ParameterList> rows) {
    impl::ParameterRows result;
    result.reserve(rows.size());
    for (const auto& row : rows) {
        result.push_back(row);
    }
    return result;
}

}  // namespace

TEST(OdbcBulkParameters, EnforcesRowShapeInPublicStore) {
    BulkParameterStore parameters;
    EXPECT_TRUE(parameters.IsEmpty());
    EXPECT_THROW(parameters.PushBackRow(), LogicError);

    parameters.PushBackRow(1, std::string{"one"});
    EXPECT_EQ(parameters.RowsCount(), 1);
    EXPECT_EQ(parameters.ColumnsCount(), 2);
    EXPECT_THROW(parameters.PushBackRow(2), LogicError);

    ParameterStore row;
    row.PushBack(2).PushBack(std::string{"two"});
    parameters.PushBackRow(std::move(row));
    EXPECT_EQ(parameters.RowsCount(), 2);
}

TEST(OdbcBulkLayout, ValidatesWholeRequestBeforeBinding) {
    const auto valid = MakeRows({
        {impl::Parameter{std::int64_t{1}}, impl::Parameter{std::optional<std::string>{}}},
        {impl::Parameter{std::uint64_t{2}}, impl::Parameter{std::string{"value"}}},
    });
    const auto layout = detail::ValidateBulkRows(valid);
    ASSERT_EQ(layout.columns.size(), 2);
    EXPECT_EQ(layout.columns[0].type, detail::BulkColumnType::kInteger);
    EXPECT_EQ(layout.columns[1].type, detail::BulkColumnType::kString);

    auto mixed = valid;
    mixed.push_back({impl::Parameter{3.0}, impl::Parameter{std::string{"three"}}});
    EXPECT_THROW(detail::ValidateBulkRows(mixed), LogicError);

    auto untyped_null = valid;
    untyped_null.push_back({impl::Parameter{3}, impl::Parameter{nullptr}});
    EXPECT_THROW(detail::ValidateBulkRows(untyped_null), LogicError);
}

TEST(OdbcBulkBindings, UsesChunkLocalStrideAndOwnsTypedNullIndicators) {
    const auto rows = MakeRows({
        {impl::Parameter{std::string{"x"}}},
        {impl::Parameter{std::string(1000, 'y')}},
        {impl::Parameter{std::optional<std::string>{}}},
    });
    const auto layout = detail::ValidateBulkRows(rows);

    auto first = detail::TryBuildBulkBindings(rows, layout, 0, 1);
    ASSERT_TRUE(first);
    ASSERT_EQ(first->columns.size(), 1);
    const auto& first_values = std::get<detail::BulkInlineValues>(first->columns[0].values);
    EXPECT_EQ(first_values.stride, 1);
    EXPECT_EQ(
        first->storage_bytes,
        sizeof(SQLCHAR) + sizeof(SQLLEN) + sizeof(SQLUSMALLINT) + sizeof(detail::BulkColumnBinding)
    );

    auto tail = detail::TryBuildBulkBindings(rows, layout, 1, 2);
    ASSERT_TRUE(tail);
    const auto& tail_values = std::get<detail::BulkInlineValues>(tail->columns[0].values);
    EXPECT_EQ(tail_values.stride, 1000);
    EXPECT_EQ(tail->columns[0].indicators[0], 1000);
    EXPECT_EQ(tail->columns[0].indicators[1], SQL_NULL_DATA);
}

TEST(OdbcBulkBindings, SharesTimestampMetadataAcrossChunk) {
    const auto rows = MakeRows({
        {impl::Parameter{Timestamp{2024, 1, 1, 0, 0, 0}}},
        {impl::Parameter{Timestamp{2024, 1, 1, 0, 0, 0, 123}}},
    });
    const auto layout = detail::ValidateBulkRows(rows);

    auto first = detail::TryBuildBulkBindings(rows, layout, 0, 1);
    ASSERT_TRUE(first);
    EXPECT_EQ(first->columns[0].column_size, 29);
    EXPECT_EQ(first->columns[0].decimal_digits, 9);
}

TEST(OdbcBulkBindings, RejectsOverBudgetPlansWithoutAllocatingPayloads) {
    const std::vector<std::size_t> multirow_slots{detail::kMaxBulkBindingBytes / 2};
    EXPECT_FALSE(detail::TryEstimateBulkBindingStorageBytes(2, multirow_slots));

    const std::vector<std::size_t> single_row_slots{detail::kMaxBulkBindingBytes + 1};
    EXPECT_FALSE(detail::TryEstimateBulkBindingStorageBytes(1, single_row_slots));

    const std::vector<std::size_t> small_slots{8, 16};
    EXPECT_TRUE(detail::TryEstimateBulkBindingStorageBytes(2, small_slots));
}

TEST(OdbcBulkStatuses, TrustsOnlyBatchCapableDriverStatuses) {
    const std::vector<SQLUSMALLINT> raw{
        SQL_PARAM_SUCCESS,
        SQL_PARAM_SUCCESS_WITH_INFO,
        SQL_PARAM_ERROR,
        SQL_PARAM_UNUSED,
        SQL_PARAM_DIAG_UNAVAILABLE,
        detail::kBulkStatusUntouched,
    };
    const auto trusted = detail::ParseBulkRowStatuses(raw, raw.size(), raw.size(), true, false);
    EXPECT_EQ(
        trusted.statuses,
        (std::vector<BulkRowStatus>{
            BulkRowStatus::kSuccess,
            BulkRowStatus::kSuccessWithInfo,
            BulkRowStatus::kError,
            BulkRowStatus::kUnused,
            BulkRowStatus::kDiagnosticsUnavailable,
            BulkRowStatus::kUnknown,
        })
    );
    EXPECT_TRUE(trusted.has_unrecognized_processed_status);

    const auto untrusted_info = detail::ParseBulkRowStatuses(raw, raw.size(), raw.size(), false, false);
    EXPECT_EQ(untrusted_info.statuses, std::vector<BulkRowStatus>(raw.size(), BulkRowStatus::kUnknown));

    const auto untrusted_success = detail::ParseBulkRowStatuses(raw, raw.size(), raw.size(), false, true);
    EXPECT_EQ(untrusted_success.statuses, std::vector<BulkRowStatus>(raw.size(), BulkRowStatus::kSuccess));
}

TEST(OdbcBulkStatuses, LeavesUntouchedTailUnknownOutsideProcessedPrefix) {
    const std::vector<SQLUSMALLINT> raw{
        SQL_PARAM_SUCCESS,
        detail::kBulkStatusUntouched,
        SQL_PARAM_UNUSED,
    };
    const auto parsed = detail::ParseBulkRowStatuses(raw, raw.size(), 1, true, false);
    EXPECT_EQ(
        parsed.statuses,
        (std::vector<BulkRowStatus>{
            BulkRowStatus::kSuccess,
            BulkRowStatus::kUnknown,
            BulkRowStatus::kUnused,
        })
    );
    EXPECT_FALSE(parsed.has_unrecognized_processed_status);
}

TEST(OdbcBulkStatuses, EvaluatesTrustReturnCodeAndProcessedCountPolicy) {
    const std::vector<SQLUSMALLINT> successful{SQL_PARAM_SUCCESS, SQL_PARAM_SUCCESS};
    const std::vector<SQLUSMALLINT> untouched{
        detail::kBulkStatusUntouched,
        detail::kBulkStatusUntouched,
    };
    const std::vector<SQLUSMALLINT> mixed{SQL_PARAM_SUCCESS, SQL_PARAM_ERROR};

    const auto trusted_success = detail::EvaluateBulkChunkStatuses(successful, 2, 2, true, SQL_SUCCESS);
    EXPECT_FALSE(trusted_success.is_failure);
    EXPECT_EQ(trusted_success.processed, 2);

    const auto trusted_unreported = detail::EvaluateBulkChunkStatuses(untouched, 2, std::nullopt, true, SQL_SUCCESS);
    EXPECT_FALSE(trusted_unreported.is_failure);
    EXPECT_FALSE(trusted_unreported.processed);
    EXPECT_EQ(trusted_unreported.statuses, std::vector<BulkRowStatus>(2, BulkRowStatus::kUnknown));

    const auto trusted_error = detail::EvaluateBulkChunkStatuses(mixed, 2, 1, true, SQL_SUCCESS_WITH_INFO);
    EXPECT_TRUE(trusted_error.is_failure);

    const auto untrusted_success = detail::EvaluateBulkChunkStatuses(mixed, 2, 2, false, SQL_SUCCESS);
    EXPECT_FALSE(untrusted_success.is_failure);
    EXPECT_EQ(untrusted_success.statuses, std::vector<BulkRowStatus>(2, BulkRowStatus::kSuccess));

    const auto untrusted_info = detail::EvaluateBulkChunkStatuses(mixed, 2, 2, false, SQL_SUCCESS_WITH_INFO);
    EXPECT_FALSE(untrusted_info.is_failure);
    EXPECT_EQ(untrusted_info.statuses, std::vector<BulkRowStatus>(2, BulkRowStatus::kUnknown));

    const auto no_data = detail::EvaluateBulkChunkStatuses(untouched, 2, 2, true, SQL_NO_DATA);
    EXPECT_FALSE(no_data.is_failure);
    EXPECT_EQ(no_data.statuses, std::vector<BulkRowStatus>(2, BulkRowStatus::kSuccess));

    const auto execution_error = detail::EvaluateBulkChunkStatuses(successful, 2, 2, true, SQL_ERROR);
    EXPECT_TRUE(execution_error.is_failure);
    EXPECT_FALSE(execution_error.processed);

    const std::vector<SQLUSMALLINT> diagnostics{
        SQL_PARAM_DIAG_UNAVAILABLE,
        SQL_PARAM_SUCCESS_WITH_INFO,
    };
    const auto diagnostics_only = detail::EvaluateBulkChunkStatuses(diagnostics, 2, 2, true, SQL_SUCCESS_WITH_INFO);
    EXPECT_FALSE(diagnostics_only.is_failure);
}

TEST(OdbcBulkResult, DerivesSucceededFromTrustedStatuses) {
    const BulkResult result{
        4,
        3,
        2,
        {
            BulkRowStatus::kSuccess,
            BulkRowStatus::kSuccessWithInfo,
            BulkRowStatus::kError,
            BulkRowStatus::kUnused,
        }
    };
    EXPECT_EQ(result.Requested(), 4);
    EXPECT_EQ(result.Processed(), 3);
    EXPECT_EQ(result.Succeeded(), 2);
    EXPECT_EQ(result.RowsAffected(), 2);
}

UTEST(OdbcBulkIntegration, EmptySingleNativeAndChunkedDml) {
    const auto host = settings::HostSettings{kDSN, {.min_size = 1, .max_size = 1}};
    Cluster cluster{settings::ODBCClusterSettings{{host}}, nullptr};
    cluster.Execute(
        ClusterHostType::kMaster,
        "CREATE TEMP TABLE odbc_bulk_integration(id BIGINT PRIMARY KEY, value TEXT)"
    );

    BulkParameterStore empty;
    const auto empty_result =
        cluster.ExecuteBulk(ClusterHostType::kMaster, "INSERT INTO odbc_bulk_integration VALUES (?, ?)", empty);
    EXPECT_EQ(empty_result.Requested(), 0);
    EXPECT_EQ(empty_result.Processed(), 0);
    EXPECT_EQ(empty_result.RowsAffected(), 0);

    BulkParameterStore single;
    single.PushBackRow(1, std::string{"one"});
    const auto single_result =
        cluster.ExecuteBulk(ClusterHostType::kMaster, "INSERT INTO odbc_bulk_integration VALUES (?, ?)", single);
    EXPECT_EQ(single_result.Requested(), 1);
    EXPECT_EQ(single_result.Processed(), 1);
    EXPECT_EQ(single_result.Succeeded(), 1);
    EXPECT_EQ(single_result.RowsAffected(), 1);

    /// [ODBC bulk DML]
    BulkParameterStore rows;
    for (std::int64_t id = 2; id <= 8; ++id) {
        rows.PushBackRow(id, std::string{"bulk-"} + std::to_string(id));
    }
    const auto result = cluster.ExecuteBulk(
        ClusterHostType::kMaster,
        Query{"INSERT INTO odbc_bulk_integration VALUES (?, ?)", Query::Name{"bulk-insert"}},
        rows,
        3
    );
    /// [ODBC bulk DML]
    EXPECT_EQ(result.Requested(), 7);
    EXPECT_EQ(result.Statuses().size(), 7);
    EXPECT_EQ(result.RowsAffected(), 7);

    const auto count = cluster.Execute(ClusterHostType::kMaster, "SELECT COUNT(*) FROM odbc_bulk_integration");
    ASSERT_EQ(count.Size(), 1);
    EXPECT_EQ(count[0][0].GetInt64(), 8);

    BulkParameterStore select_row;
    select_row.PushBackRow(1);
    EXPECT_THROW(cluster.ExecuteBulk(ClusterHostType::kMaster, "SELECT ?::integer", select_row), StatementError);

    cluster.Execute(
        ClusterHostType::kMaster,
        "CREATE TEMP TABLE odbc_bulk_buffers(id BIGINT PRIMARY KEY, value TEXT NULL, payload BYTEA)"
    );
    BulkParameterStore buffers;
    buffers.PushBackRow(1, std::optional<std::string>{}, Bytes{0, 1, 0, 2})
        .PushBackRow(2, std::optional<std::string>{"quoted ' value"}, Bytes{});
    const auto buffer_result =
        cluster.ExecuteBulk(ClusterHostType::kMaster, "INSERT INTO odbc_bulk_buffers VALUES (?, ?, ?)", buffers, 2);
    EXPECT_EQ(buffer_result.Requested(), 2);
    const auto stored_buffers = cluster.Execute(
        ClusterHostType::kMaster,
        "SELECT value IS NULL, octet_length(payload) FROM odbc_bulk_buffers ORDER BY id"
    );
    ASSERT_EQ(stored_buffers.Size(), 2);
    EXPECT_TRUE(stored_buffers[0][0].GetBool());
    EXPECT_EQ(stored_buffers[0][1].GetInt32(), 4);
    EXPECT_FALSE(stored_buffers[1][0].GetBool());
    EXPECT_EQ(stored_buffers[1][1].GetInt32(), 0);

    BulkParameterStore zero_updates;
    zero_updates.PushBackRow(std::string{"missing"}, 100).PushBackRow(std::string{"missing"}, 101);
    const auto zero_update_result = cluster.ExecuteBulk(
        ClusterHostType::kMaster,
        "UPDATE odbc_bulk_buffers SET value = ? WHERE id = ?",
        zero_updates,
        2
    );
    EXPECT_EQ(zero_update_result.RowsAffected(), 0);

    BulkParameterStore zero_deletes;
    zero_deletes.PushBackRow(100).PushBackRow(101);
    const auto zero_delete_result =
        cluster.ExecuteBulk(ClusterHostType::kMaster, "DELETE FROM odbc_bulk_buffers WHERE id = ?", zero_deletes, 2);
    EXPECT_EQ(zero_delete_result.RowsAffected(), 0);
}

UTEST(OdbcBulkIntegration, ReportsPartialFailureAndTransactionRollback) {
    const auto host = settings::HostSettings{kDSN, {.min_size = 1, .max_size = 1}};
    Cluster cluster{settings::ODBCClusterSettings{{host}}, nullptr};
    cluster.Execute(ClusterHostType::kMaster, "CREATE TEMP TABLE odbc_bulk_partial(id BIGINT PRIMARY KEY)");

    BulkParameterStore duplicate_rows;
    duplicate_rows.PushBackRow(10).PushBackRow(11).PushBackRow(10).PushBackRow(12);
    try {
        cluster.ExecuteBulk(ClusterHostType::kMaster, "INSERT INTO odbc_bulk_partial VALUES (?)", duplicate_rows, 4);
        FAIL() << "A duplicate bulk row must fail";
    } catch (const BulkExecutionError& error) {
        EXPECT_EQ(error.GetResult().Requested(), 4);
        EXPECT_EQ(error.GetResult().Statuses().size(), 4);
        EXPECT_LT(error.GetResult().Succeeded(), 4);
    }

    auto transaction = cluster.Begin(ClusterHostType::kMaster);
    BulkParameterStore transaction_rows;
    transaction_rows.PushBackRow(20).PushBackRow(21).PushBackRow(22);
    const auto
        transaction_result = transaction.ExecuteBulk("INSERT INTO odbc_bulk_partial VALUES (?)", transaction_rows, 2);
    EXPECT_EQ(transaction_result.Requested(), 3);
    transaction.Rollback();

    const auto rolled_back =
        cluster.Execute(ClusterHostType::kMaster, "SELECT COUNT(*) FROM odbc_bulk_partial WHERE id >= 20");
    ASSERT_EQ(rolled_back.Size(), 1);
    EXPECT_EQ(rolled_back[0][0].GetInt64(), 0);

    BulkParameterStore empty;
    EXPECT_THROW(transaction.ExecuteBulk("INSERT INTO odbc_bulk_partial VALUES (?)", empty), TransactionException);
}

UTEST(OdbcBulkIntegration, ReusesPreparedCacheAfterCompleteCleanup) {
    const auto host = settings::HostSettings{kDSN, {.min_size = 1, .max_size = 1}};
    Cluster cluster{settings::ODBCClusterSettings{{host}}, nullptr};
    cluster.SetPreparedStatementCacheSettings({.max_size = 1});
    cluster.Execute(ClusterHostType::kMaster, "CREATE TEMP TABLE odbc_bulk_cache(id BIGINT PRIMARY KEY)");

    utils::statistics::Storage statistics_storage;
    auto entry = statistics_storage.RegisterWriter("odbc", [&cluster](utils::statistics::Writer& writer) {
        cluster.WriteStatistics(writer);
    });

    auto transaction = cluster.Begin(ClusterHostType::kMaster);
    const Query insert{"INSERT INTO odbc_bulk_cache VALUES (?)"};
    transaction.Execute(insert, 0);
    BulkParameterStore first;
    first.PushBackRow(1).PushBackRow(2);
    transaction.ExecuteBulk(insert, first);
    BulkParameterStore wrong_arity;
    wrong_arity.PushBackRow(100, 200);
    EXPECT_THROW(transaction.ExecuteBulk(insert, wrong_arity), StatementError);
    transaction.Execute(insert, 3);
    BulkParameterStore second;
    second.PushBackRow(4).PushBackRow(5);
    transaction.ExecuteBulk(insert, second);
    transaction.Commit();

    const utils::statistics::Snapshot snapshot{statistics_storage, "odbc", {{"odbc_pool", "0"}}};
    EXPECT_EQ(snapshot.SingleMetric("queries.prepared-cache-misses").AsRate(), 2);
    EXPECT_EQ(snapshot.SingleMetric("queries.prepared-cache-hits").AsRate(), 3);
    const auto count = cluster.Execute(ClusterHostType::kMaster, "SELECT COUNT(*) FROM odbc_bulk_cache");
    EXPECT_EQ(count[0][0].GetInt64(), 6);
    entry.Unregister();
}

UTEST(OdbcBulkIntegration, TimeoutDoesNotRetryAndPoolRecovers) {
    const auto host = settings::HostSettings{kDSN, {.min_size = 1, .max_size = 1}};
    Cluster cluster{settings::ODBCClusterSettings{{host}}, nullptr};
    cluster.Execute(ClusterHostType::kMaster, "DROP TABLE IF EXISTS odbc_bulk_timeout");
    cluster.Execute(ClusterHostType::kMaster, "CREATE TABLE odbc_bulk_timeout(id BIGINT PRIMARY KEY)");

    BulkParameterStore rows;
    rows.PushBackRow(1).PushBackRow(2);
    EXPECT_THROW(
        cluster.ExecuteBulk(
            ClusterHostType::kMaster,
            CommandControl{.statement_timeout = 1ms},
            "INSERT INTO odbc_bulk_timeout SELECT ?::bigint FROM pg_sleep(0.05)",
            rows,
            2
        ),
        OperationInterrupted
    );

    // The timed-out HDBC is discarded. Depending on when the driver observes
    // cancellation, zero, one, or both distinct rows may have committed; none
    // is retried. A fresh pooled connection must remain usable.
    cluster.Execute(ClusterHostType::kMaster, "INSERT INTO odbc_bulk_timeout VALUES (99)");
    const auto result = cluster.Execute(
        ClusterHostType::kMaster,
        "SELECT COUNT(*), COUNT(DISTINCT id) FROM odbc_bulk_timeout WHERE id IN (1, 2)"
    );
    ASSERT_EQ(result.Size(), 1);
    EXPECT_LE(result[0][0].GetInt64(), 2);
    EXPECT_EQ(result[0][0].GetInt64(), result[0][1].GetInt64());
    cluster.Execute(ClusterHostType::kMaster, "DROP TABLE odbc_bulk_timeout");
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END
