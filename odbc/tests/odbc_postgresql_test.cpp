#include <gtest/gtest.h>
#include <concepts>
#include <cstdint>
#include <limits>
#include <optional>
#include <storages/odbc/detail/connection.hpp>
#include <storages/odbc/detail/tracing.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <userver/storages/odbc.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <vector>

#include <userver/storages/odbc/tests/utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests {

using ClusterParameterStoreExecute = ResultSet (Cluster::*)(ClusterHostTypeFlags, const Query&, const ParameterStore&);
using ClusterParameterStoreExecuteWithCommandControl =
    ResultSet (Cluster::*)(ClusterHostTypeFlags, OptionalCommandControl, const Query&, const ParameterStore&);
using TransactionParameterStoreExecute = ResultSet (Transaction::*)(const Query&, const ParameterStore&);
using TransactionParameterStoreExecuteWithCommandControl =
    ResultSet (Transaction::*)(OptionalCommandControl, const Query&, const ParameterStore&);

template <typename T>
concept ParameterStorePushable = requires(ParameterStore& store, const T& value) { store.PushBack(value); };

struct UnsupportedParameter final {};

static_assert(!std::is_copy_constructible_v<ParameterStore>);
static_assert(!std::is_copy_assignable_v<ParameterStore>);
static_assert(std::is_nothrow_move_constructible_v<ParameterStore>);
static_assert(std::is_nothrow_move_assignable_v<ParameterStore>);
static_assert(requires(ParameterStore& store, const std::optional<std::int32_t>& value) {
    {
        store.PushBack(value)
    } -> std::same_as<ParameterStore&>;
    store.PushBack("string literal");
});
static_assert(ParameterStorePushable<std::nullptr_t>);
static_assert(ParameterStorePushable<std::nullopt_t>);
static_assert(ParameterStorePushable<const char*>);
static_assert(ParameterStorePushable<std::string_view>);
static_assert(ParameterStorePushable<std::optional<std::string>>);
static_assert(ParameterStorePushable<Bytes>);
static_assert(ParameterStorePushable<Date>);
static_assert(ParameterStorePushable<Time>);
static_assert(ParameterStorePushable<Timestamp>);
static_assert(ParameterStorePushable<Decimal<9, 4>>);
static_assert(ParameterStorePushable<std::optional<Bytes>>);
static_assert(ParameterStorePushable<std::optional<Decimal<9, 4>>>);
static_assert(!ParameterStorePushable<void*>);
static_assert(!ParameterStorePushable<UnsupportedParameter>);
static_assert(!ParameterStorePushable<std::optional<std::vector<int>>>);
static_assert(requires {
    static_cast<ClusterParameterStoreExecute>(&Cluster::Execute);
    static_cast<ClusterParameterStoreExecuteWithCommandControl>(&Cluster::Execute);
    static_cast<TransactionParameterStoreExecute>(&Transaction::Execute);
    static_cast<TransactionParameterStoreExecuteWithCommandControl>(&Transaction::Execute);
});

UTEST(CreateConnection, Works) { auto cluster = MakeCluster(); }

UTEST(CreateConnection, MultipleDSN) { auto cluster = MakeCluster(kMultiDSNSettings); }

UTEST(DriverCapabilities, CapturesPsqlOdbcSnapshot) {
    Connection connection{kDSN};
    const auto& capabilities = connection.GetDriverCapabilities();

    EXPECT_EQ(capabilities.GetDbmsName(), "PostgreSQL");
    EXPECT_FALSE(capabilities.GetDbmsVersion().empty());
    EXPECT_NE(capabilities.GetDriverName().find("psqlodbc"), std::string::npos);
    EXPECT_FALSE(capabilities.GetDriverVersion().empty());
    EXPECT_FALSE(capabilities.GetDriverOdbcVersion().empty());

    const auto transaction_capability = capabilities.GetTransactionCapability();
    ASSERT_TRUE(transaction_capability);
    EXPECT_NE(*transaction_capability, detail::TransactionCapability::kNone);

    const auto isolation_options = capabilities.GetTransactionIsolationOptions();
    ASSERT_TRUE(isolation_options);
    EXPECT_NE(*isolation_options & SQL_TXN_READ_COMMITTED, 0U);
    const auto default_isolation = capabilities.GetDefaultTransactionIsolation();
    ASSERT_TRUE(default_isolation);
    EXPECT_NE(*default_isolation, 0U);
    EXPECT_EQ(*default_isolation & *isolation_options, *default_isolation);

    ASSERT_TRUE(capabilities.IsDataSourceReadOnly());
    EXPECT_FALSE(*capabilities.IsDataSourceReadOnly());
    ASSERT_TRUE(capabilities.CanDescribeParameters());
    EXPECT_FALSE(*capabilities.CanDescribeParameters());

    EXPECT_TRUE(capabilities.GetParameterArrayRowCounts());
    EXPECT_TRUE(capabilities.GetParameterArraySelects());
    EXPECT_TRUE(capabilities.GetBatchRowCount());

    const auto scroll_options = capabilities.GetScrollOptions();
    ASSERT_TRUE(scroll_options);
    EXPECT_NE(*scroll_options & SQL_SO_FORWARD_ONLY, 0U);
    EXPECT_TRUE(capabilities.GetGetDataExtensions());
    EXPECT_TRUE(capabilities.GetCursorCommitBehavior());
    EXPECT_TRUE(capabilities.GetCursorRollbackBehavior());
}

UTEST(Query, Works) {
    auto cluster = MakeCluster();

    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
    EXPECT_EQ(result.Size(), 1);
    EXPECT_FALSE(result.IsEmpty());
    auto row = result[0];
    EXPECT_EQ(row.Size(), 1);
    EXPECT_FALSE(row[0].IsNull());
    EXPECT_EQ(row[0].GetInt32(), 1);

    auto multipleRows = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT generate_series(1, 10)");
    EXPECT_EQ(multipleRows.Size(), 10);
    for (std::size_t i = 0; i < multipleRows.Size(); i++) {
        auto row = multipleRows[i];
        EXPECT_EQ(row.Size(), 1);
        EXPECT_FALSE(row[0].IsNull());
        EXPECT_EQ(row[0].GetInt32(), i + 1);
    }
}

UTEST(Query, BindsParametersWithoutInterpolation) {
    auto cluster = MakeCluster();

    /// [ODBC parameter binding]
    const std::string untrusted_value = "Robert'); DROP TABLE users;--";
    const auto result = cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "SELECT ?::text, ?::text, ?::bigint, ?::bigint, ?::double precision, ?::boolean, ?::boolean",
        untrusted_value,
        std::string_view{""},
        std::int16_t{-42},
        std::uint32_t{42},
        1.25F,
        true,
        false
    );
    /// [ODBC parameter binding]

    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetString(), untrusted_value);
    EXPECT_EQ(result[0][1].GetString(), "");
    EXPECT_EQ(result[0][2].GetInt64(), -42);
    EXPECT_EQ(result[0][3].GetInt64(), 42);
    EXPECT_DOUBLE_EQ(result[0][4].GetDouble(), 1.25);
    EXPECT_TRUE(result[0][5].GetBool());
    EXPECT_FALSE(result[0][6].GetBool());
}

UTEST(ParameterStore, OwnsDynamicParametersAndIsReusable) {
    auto cluster = MakeCluster();

    ParameterStore empty;
    EXPECT_TRUE(empty.IsEmpty());
    EXPECT_EQ(empty.Size(), 0);
    const auto empty_result = cluster.Execute(ClusterHostType::kMaster, "SELECT 1", empty);
    ASSERT_EQ(empty_result.Size(), 1);
    EXPECT_EQ(empty_result[0][0].GetInt32(), 1);

    /// [ODBC dynamic parameter store]
    const std::string injection_payload = "Robert'); DROP TABLE users;--";
    std::string copied_source = injection_payload;
    const std::optional<std::int32_t> null_integer;
    const std::optional<std::string> null_string;

    ParameterStore parameters;
    parameters.PushBack(std::int64_t{42})
        .PushBack(copied_source)
        .PushBack("string literal")
        .PushBack(std::string{"temporary string"})
        .PushBack(null_integer)
        .PushBack(null_string);

    const Query query{
        "SELECT ?::bigint, ?::text, ?::text, ?::text, ?::integer IS NULL, ?::text IS NULL",
    };
    const auto result = cluster.Execute(ClusterHostType::kMaster, query, parameters);
    /// [ODBC dynamic parameter store]

    copied_source.assign("changed after PushBack");
    EXPECT_FALSE(parameters.IsEmpty());
    EXPECT_EQ(parameters.Size(), 6);

    ParameterStore moved_parameters{std::move(parameters)};
    EXPECT_EQ(moved_parameters.Size(), 6);

    const auto validate = [&injection_payload](const ResultSet& value) {
        ASSERT_EQ(value.Size(), 1);
        EXPECT_EQ(value[0][0].GetInt64(), 42);
        EXPECT_EQ(value[0][1].GetString(), injection_payload);
        EXPECT_EQ(value[0][2].GetString(), "string literal");
        EXPECT_EQ(value[0][3].GetString(), "temporary string");
        EXPECT_TRUE(value[0][4].GetBool());
        EXPECT_TRUE(value[0][5].GetBool());
    };
    validate(result);

    // Reusing a store never consumes or mutates its values.
    validate(cluster.Execute(ClusterHostType::kMaster, CommandControl{}, query, moved_parameters));

    auto transaction = cluster.Begin(ClusterHostType::kMaster);
    validate(transaction.Execute(query, moved_parameters));
    validate(transaction.Execute(CommandControl{}, query, moved_parameters));
    UEXPECT_THROW(transaction.Execute("SELECT ?::integer", moved_parameters), StatementError);
    transaction.Rollback();

    // Unlike raw nullptr, a null const char* retains the string parameter type.
    const char* null_c_string = nullptr;
    ParameterStore null_string_parameter;
    null_string_parameter.PushBack(null_c_string);
    const auto null_result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::text IS NULL", null_string_parameter);
    ASSERT_EQ(null_result.Size(), 1);
    EXPECT_TRUE(null_result[0][0].GetBool());
}

UTEST(ParameterStore, PreservesUnsignedBigintRangeChecks) {
    auto cluster = MakeCluster();

    ParameterStore supported;
    supported.PushBack(static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()));
    const auto result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::bigint", supported);
    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetInt64(), std::numeric_limits<std::int64_t>::max());

    ParameterStore unsupported;
    unsupported.PushBack(std::numeric_limits<std::uint64_t>::max());
    UEXPECT_THROW(cluster.Execute(ClusterHostType::kMaster, "SELECT ?::numeric", unsupported), StatementError);
}

UTEST(Query, RejectsUnsignedValuesOutsidePortableBigintRange) {
    auto cluster = MakeCluster();

    const auto supported = cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "SELECT ?::bigint",
        static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())
    );
    ASSERT_EQ(supported.Size(), 1);
    EXPECT_EQ(supported[0][0].GetInt64(), std::numeric_limits<std::int64_t>::max());

    UEXPECT_THROW(
        cluster.Execute(
            storages::odbc::ClusterHostType::kMaster,
            "SELECT ?::numeric",
            std::numeric_limits<std::uint64_t>::max()
        ),
        storages::odbc::StatementError
    );
}

UTEST(Query, HonorsQueryLogModeAndName) {
    const Query named{
        "SELECT 'must not leak'",
        Query::Name{"safe_query_name"},
        Query::LogMode::kNameOnly,
    };
    const auto named_tags = detail::tracing::MakeQuerySpanTags(named);
    ASSERT_TRUE(named_tags.statement_name);
    EXPECT_EQ(*named_tags.statement_name, "safe_query_name");
    EXPECT_FALSE(named_tags.statement);

    const Query unnamed_name_only{
        "SELECT 'must not leak either'",
        std::nullopt,
        Query::LogMode::kNameOnly,
    };
    const auto hidden_tags = detail::tracing::MakeQuerySpanTags(unnamed_name_only);
    EXPECT_FALSE(hidden_tags.statement_name);
    EXPECT_FALSE(hidden_tags.statement);

    const Query unnamed_full{"SELECT 1"};
    const auto full_tags = detail::tracing::MakeQuerySpanTags(unnamed_full);
    EXPECT_FALSE(full_tags.statement_name);
    ASSERT_TRUE(full_tags.statement);
    EXPECT_EQ(*full_tags.statement, "SELECT 1");
}

UTEST(Query, BindsTypedNull) {
    auto cluster = MakeCluster();

    const std::optional<std::string> value;
    const auto result = cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "SELECT ?::text IS NULL, ?::text IS NULL",
        value,
        nullptr
    );

    ASSERT_EQ(result.Size(), 1);
    EXPECT_TRUE(result[0][0].GetBool());
    EXPECT_TRUE(result[0][1].GetBool());
}

UTEST(Query, BindsAndReadsPortableStandardTypes) {
    /// [ODBC portable standard types]
    auto cluster = MakeCluster();
    const Bytes bytes{{0, 1, 2, 0, 255}};
    const Date date{2024, 2, 29};
    const Time time{23, 58, 57};
    const Timestamp timestamp{2024, 2, 29, 23, 58, 57, 123'456'000};
    const Decimal<9, 4> decimal{"+00123.4500"};

    const auto bytes_result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::bytea", bytes);
    const auto date_result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::date", date);
    const auto time_result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::time", time);
    const auto timestamp_result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::timestamp", timestamp);
    const auto decimal_result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::numeric(9,4)", decimal);
    /// [ODBC portable standard types]

    ASSERT_EQ(bytes_result.Size(), 1);
    EXPECT_EQ(bytes_result[0][0].As<Bytes>(), bytes);
    EXPECT_EQ(bytes_result[0][0].GetString().size(), bytes.Size());

    ASSERT_EQ(date_result.Size(), 1);
    EXPECT_EQ(date_result[0][0].As<Date>(), date);
    EXPECT_EQ(date_result[0][0].GetString(), "2024-02-29");

    ASSERT_EQ(time_result.Size(), 1);
    EXPECT_EQ(time_result[0][0].As<Time>(), time);
    EXPECT_EQ(time_result[0][0].GetString(), "23:58:57");

    ASSERT_EQ(timestamp_result.Size(), 1);
    EXPECT_EQ(timestamp_result[0][0].As<Timestamp>(), timestamp);
    EXPECT_EQ(timestamp_result[0][0].GetString(), "2024-02-29 23:58:57.123456000");

    ASSERT_EQ(decimal_result.Size(), 1);
    EXPECT_EQ((decimal_result[0][0].As<Decimal<9, 4>>()), decimal);
    EXPECT_EQ(decimal_result[0][0].GetString(), "123.4500");
}

UTEST(ParameterStore, BindsPortableStandardTypesAndTypedNulls) {
    auto cluster = MakeCluster();
    const Bytes bytes{{7, 0, 8}};
    const Date date{2000, 1, 2};
    const Time time{3, 4, 5};
    const Timestamp timestamp{2000, 1, 2, 3, 4, 5, 987'654'000};
    const Decimal<9, 4> decimal{"-12.3400"};

    ParameterStore bytes_parameters;
    bytes_parameters.PushBack(bytes).PushBack(std::optional<Bytes>{});
    const auto
        bytes_result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::bytea, ?::bytea IS NULL", bytes_parameters);
    ASSERT_EQ(bytes_result.Size(), 1);
    EXPECT_EQ(bytes_result[0][0].As<Bytes>(), bytes);
    EXPECT_TRUE(bytes_result[0][1].GetBool());

    ParameterStore date_parameters;
    date_parameters.PushBack(date).PushBack(std::optional<Date>{});
    const auto
        date_result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::date, ?::date IS NULL", date_parameters);
    ASSERT_EQ(date_result.Size(), 1);
    EXPECT_EQ(date_result[0][0].As<Date>(), date);
    EXPECT_TRUE(date_result[0][1].GetBool());

    ParameterStore time_parameters;
    time_parameters.PushBack(time).PushBack(std::optional<Time>{});
    const auto
        time_result = cluster.Execute(ClusterHostType::kMaster, "SELECT ?::time, ?::time IS NULL", time_parameters);
    ASSERT_EQ(time_result.Size(), 1);
    EXPECT_EQ(time_result[0][0].As<Time>(), time);
    EXPECT_TRUE(time_result[0][1].GetBool());

    ParameterStore timestamp_parameters;
    timestamp_parameters.PushBack(timestamp).PushBack(std::optional<Timestamp>{});
    const auto timestamp_result =
        cluster.Execute(ClusterHostType::kMaster, "SELECT ?::timestamp, ?::timestamp IS NULL", timestamp_parameters);
    ASSERT_EQ(timestamp_result.Size(), 1);
    EXPECT_EQ(timestamp_result[0][0].As<Timestamp>(), timestamp);
    EXPECT_TRUE(timestamp_result[0][1].GetBool());

    ParameterStore decimal_parameters;
    decimal_parameters.PushBack(decimal).PushBack(std::optional<Decimal<9, 4>>{});
    const auto decimal_result =
        cluster
            .Execute(ClusterHostType::kMaster, "SELECT ?::numeric(9,4), ?::numeric(9,4) IS NULL", decimal_parameters);
    ASSERT_EQ(decimal_result.Size(), 1);
    EXPECT_EQ((decimal_result[0][0].As<Decimal<9, 4>>()), decimal);
    EXPECT_TRUE(decimal_result[0][1].GetBool());
}

UTEST(Query, MaterializesBinaryChunkBoundariesWithoutTerminatorHeuristics) {
    auto cluster = MakeCluster();
    const auto make_bytes = [](std::size_t size, std::uint8_t seed) {
        Bytes::Container value(size);
        for (std::size_t index = 0; index < value.size(); ++index) {
            value[index] = static_cast<std::uint8_t>(seed + index);
        }
        return Bytes{std::move(value)};
    };
    const Bytes empty;
    const auto bytes_4095 = make_bytes(4095, 1);
    const auto bytes_4096 = make_bytes(4096, 2);
    const auto bytes_4097 = make_bytes(4097, 3);
    const auto bytes_long = make_bytes(65'537, 4);

    const auto result = cluster.Execute(
        ClusterHostType::kMaster,
        "SELECT ?::bytea, ?::bytea, ?::bytea, ?::bytea, ?::bytea",
        empty,
        bytes_4095,
        bytes_4096,
        bytes_4097,
        bytes_long
    );

    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].As<Bytes>(), empty);
    EXPECT_EQ(result[0][1].As<Bytes>(), bytes_4095);
    EXPECT_EQ(result[0][2].As<Bytes>(), bytes_4096);
    EXPECT_EQ(result[0][3].As<Bytes>(), bytes_4097);
    EXPECT_EQ(result[0][4].As<Bytes>(), bytes_long);
}

UTEST(Query, ParameterCountMismatchIsStatementError) {
    auto cluster = MakeCluster();
    UEXPECT_THROW(
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT ?::integer", 1, 2),
        storages::odbc::StatementError
    );
}

UTEST(Query, VariousTypes) {
    auto query = "SELECT 42, 'test', 1.0, false, null, true";
    auto cluster = MakeCluster();

    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, query);
    EXPECT_EQ(result.Size(), 1);
    EXPECT_FALSE(result.IsEmpty());
    auto row = result[0];
    EXPECT_EQ(row.Size(), 6);

    auto intField = row[0];
    EXPECT_EQ(intField.GetInt32(), 42);

    auto stringField = row[1];
    EXPECT_EQ(stringField.GetString(), "test");

    auto doubleField = row[2];
    EXPECT_DOUBLE_EQ(doubleField.GetDouble(), 1.0);

    auto boolField = row[3];
    EXPECT_EQ(boolField.GetBool(), false);

    auto nullField = row[4];
    EXPECT_TRUE(nullField.IsNull());

    auto trueBool = row[5];
    EXPECT_EQ(trueBool.GetBool(), true);
}

UTEST(Query, DifferentHostTypes) {
    auto query = "SELECT 1";
    auto cluster = MakeCluster();

    // TODO: needs an actual check that host are selected correctly
    cluster.Execute(storages::odbc::ClusterHostType::kMaster, query);
    cluster.Execute(storages::odbc::ClusterHostType::kSlave, query);
    cluster.Execute(storages::odbc::ClusterHostType::kNone, query);
}

UTEST(Query, EmptyResult) {
    auto cluster = MakeCluster();
    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1 WHERE false");
    EXPECT_EQ(result.Size(), 0);
    EXPECT_TRUE(result.IsEmpty());
}

UTEST(Query, FieldCount) {
    auto cluster = MakeCluster();
    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1, 2, 3");
    EXPECT_EQ(result.FieldCount(), 3);
    EXPECT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0].Size(), 3);
}

UTEST(Query, GetInt64) {
    auto cluster = MakeCluster();
    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 2147483648");
    EXPECT_EQ(result[0][0].GetInt64(), 2147483648LL);
}

UTEST(Query, GetStringFromNumber) {
    auto cluster = MakeCluster();
    auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT '42'");
    EXPECT_EQ(result[0][0].GetString(), "42");
}

UTEST(Query, MaterializedResultOutlivesConnectionAndSubsequentQuery) {
    const auto host_settings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    std::optional<ResultSet> saved_result;
    {
        storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{host_settings}}, nullptr);
        saved_result
            .emplace(cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT ?::text AS saved_value", "first")
            );

        const auto second = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT ?::integer", 2);
        ASSERT_EQ(second.Size(), 1);
        EXPECT_EQ(second[0][0].GetInt32(), 2);
    }

    ASSERT_TRUE(saved_result);
    ASSERT_EQ(saved_result->Size(), 1);
    EXPECT_EQ(saved_result->GetFieldName(0), "saved_value");
    EXPECT_EQ((*saved_result)[0][0].GetString(), "first");
}

UTEST(Query, MaterializesLongEmptyAndNullValues) {
    auto cluster = MakeCluster();
    const std::string long_value(70'000, 'x');

    const auto result = cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "SELECT ?::text AS long_value, ?::text AS empty_value, NULL::text AS null_value",
        long_value,
        std::string{}
    );

    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetString(), long_value);
    EXPECT_EQ(result[0][1].GetString(), "");
    EXPECT_TRUE(result[0][2].IsNull());
    UEXPECT_THROW(result[0][2].GetString(), storages::odbc::ResultSetError);
}

UTEST(Query, MaterializesChunkBoundariesAndTypedNulls) {
    auto cluster = MakeCluster();
    const std::string value_4095(4095, 'a');
    const std::string value_4096(4096, 'b');
    const std::string value_4097(4097, 'c');
    const std::string value_65537(65'537, 'd');

    const auto result = cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "SELECT ?::text, ?::text, ?::text, ?::text, NULL::integer, NULL::boolean, NULL::double precision",
        value_4095,
        value_4096,
        value_4097,
        value_65537
    );

    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetString(), value_4095);
    EXPECT_EQ(result[0][1].GetString(), value_4096);
    EXPECT_EQ(result[0][2].GetString(), value_4097);
    EXPECT_EQ(result[0][3].GetString(), value_65537);
    for (std::size_t index = 4; index < 7; ++index) {
        EXPECT_TRUE(result[0][index].IsNull());
    }
    UEXPECT_THROW(result[0][4].GetInt32(), storages::odbc::ResultSetError);
    UEXPECT_THROW(result[0][5].GetBool(), storages::odbc::ResultSetError);
    UEXPECT_THROW(result[0][6].GetDouble(), storages::odbc::ResultSetError);
}

UTEST(Query, MaterializedResultSurvivesTopologyReload) {
    const auto host_settings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{host_settings}}, nullptr);
    const auto result = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT ?::text", "before reload");

    const std::string reloaded_dsn = std::string{kDSN} + "ApplicationName=odbc-materialized-result;";
    cluster.UpdateSettings(storages::odbc::settings::ODBCClusterSettings{{
        storages::odbc::settings::HostSettings{reloaded_dsn, {1, 1}},
    }});

    ASSERT_EQ(result.Size(), 1);
    EXPECT_EQ(result[0][0].GetString(), "before reload");
}

UTEST(Query, SeparatesRowsFromRowsAffected) {
    const auto host_settings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{host_settings}}, nullptr);
    cluster.Execute(storages::odbc::ClusterHostType::kMaster, "CREATE TEMP TABLE odbc_rows_affected(value INTEGER)");

    const auto insert = cluster.Execute(
        storages::odbc::ClusterHostType::kMaster,
        "INSERT INTO odbc_rows_affected(value) VALUES (?), (?)",
        1,
        2
    );
    EXPECT_EQ(insert.Size(), 0);
    EXPECT_EQ(insert.RowsAffected(), 2);

    const auto select =
        cluster
            .Execute(storages::odbc::ClusterHostType::kMaster, "SELECT value FROM odbc_rows_affected ORDER BY value");
    EXPECT_EQ(select.Size(), 2);
    EXPECT_EQ(select.RowsAffected(), 0);
}

UTEST(Pool, LessQueriesThanConnections) {
    std::size_t poolConnections = 5;
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {poolConnections, poolConnections}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}}, nullptr);

    std::vector<engine::TaskWithResult<ResultSet>> futures;
    futures.reserve(poolConnections);

    for (std::size_t i = 0; i < poolConnections - 1; i++) {
        futures.emplace_back(utils::Async("LessQueriesThanConnections", [&cluster]() {
            return cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
        }));
    }

    for (auto& future : futures) {
        auto result = future.Get();
        EXPECT_EQ(result.Size(), 1);
        EXPECT_EQ(result[0][0].GetInt32(), 1);
    }
}

UTEST(Pool, EqualQueriesAndConnections) {
    std::size_t poolConnections = 5;
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {poolConnections, poolConnections}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}}, nullptr);

    std::vector<engine::TaskWithResult<ResultSet>> futures;
    futures.reserve(poolConnections);

    for (std::size_t i = 0; i < poolConnections; i++) {
        futures.emplace_back(utils::Async("EqualQueriesAndConnections", [&cluster]() {
            return cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
        }));
    }

    for (auto& future : futures) {
        auto result = future.Get();
        EXPECT_EQ(result.Size(), 1);
        EXPECT_EQ(result[0][0].GetInt32(), 1);
    }
}

UTEST(Pool, MoreQueriesThanConnectionsButLessThanPoolSize) {
    std::size_t poolConnections = 5;
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {poolConnections, poolConnections * 2}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}}, nullptr);

    std::vector<engine::TaskWithResult<ResultSet>> futures;
    futures.reserve(poolConnections + 2);

    for (std::size_t i = 0; i < poolConnections + 2; i++) {
        futures.emplace_back(utils::Async("MoreQueriesThanConnectionsButLessThanPoolSize", [&cluster]() {
            return cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
        }));
    }

    for (auto& future : futures) {
        auto result = future.Get();
        EXPECT_EQ(result.Size(), 1);
        EXPECT_EQ(result[0][0].GetInt32(), 1);
    }
}

UTEST(Pool, MoreQueriesThanConnectionsAndPoolSize) {
    std::size_t poolConnections = 5;
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {poolConnections, poolConnections}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}}, nullptr);

    std::vector<engine::TaskWithResult<ResultSet>> futures;
    futures.reserve(poolConnections * 2);

    for (std::size_t i = 0; i < poolConnections * 2; i++) {
        futures.emplace_back(utils::Async("MoreQueriesThanConnectionsAndPoolSize", [&cluster]() {
            return cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
        }));
    }

    for (auto& future : futures) {
        auto result = future.Get();
        EXPECT_EQ(result.Size(), 1);
        EXPECT_EQ(result[0][0].GetInt32(), 1);
    }
}

UTEST(Pool, RestoresBrokenConnection) {
    auto hostSettings = storages::odbc::settings::HostSettings{kDSN, {1, 1}};
    storages::odbc::Cluster cluster(storages::odbc::settings::ODBCClusterSettings{{hostSettings}}, nullptr);

    auto killConnectionQuery = "SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname = 'postgres';";

    try {
        cluster.Execute(storages::odbc::ClusterHostType::kMaster, killConnectionQuery);
    } catch (...) {
    }

    auto selectRes = cluster.Execute(storages::odbc::ClusterHostType::kMaster, "SELECT 1");
    EXPECT_EQ(selectRes.Size(), 1);
    EXPECT_EQ(selectRes[0][0].GetInt32(), 1);
}

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END
