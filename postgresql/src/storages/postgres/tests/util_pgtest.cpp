#include <storages/postgres/tests/util_pgtest.hpp>

#include <boost/algorithm/string.hpp>

#include <userver/concurrent/background_task_storage.hpp>
#include <userver/utils/statistics/metrics_storage.hpp>

#include <storages/postgres/default_command_controls.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <userver/storages/postgres/exceptions.hpp>

#include <fmt/format.h>

#include <iomanip>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

namespace {
constexpr const char* kPostgresDsn = "POSTGRES_TEST_DSN";

std::string GetFirstValue(std::string_view values) {
    const auto pos = values.find(',');
    return std::string{values.substr(0, pos)};
}

std::uint16_t GetPostgresPort(const pg::Dsn& dsn) {
    const auto options = pg::OptionsFromDsn(dsn);
    if (options.port.empty()) {
        return 5432;
    }

    return static_cast<std::uint16_t>(std::stoi(GetFirstValue(options.port)));
}

std::string GetPostgresHost(const pg::Dsn& dsn) {
    const auto options = pg::OptionsFromDsn(dsn);
    if (options.host.empty()) {
        return "127.0.0.1";
    }

    return GetFirstValue(options.host);
}

pg::Dsn MakeProxiedDsn(const pg::Dsn& dsn, std::uint16_t proxy_port) {
    const auto& dsn_str = dsn.GetUnderlying();
    const auto at_pos = dsn_str.find('@');
    const auto colon_pos = dsn_str.find(':', at_pos + 1);
    const auto slash_pos = dsn_str.find('/', colon_pos + 1);

    UASSERT_MSG(
        at_pos != std::string::npos && colon_pos != std::string::npos && slash_pos != std::string::npos,
        "PG connection string is expected in format ...@<hostname>:<port>/..."
    );

    return pg::Dsn{fmt::format("{}127.0.0.1:{}{}", dsn_str.substr(0, at_pos + 1), proxy_port, dsn_str.substr(slash_pos))
    };
}

}  // namespace

pg::DefaultCommandControls GetTestCmdCtls() {
    static auto default_cmd_ctls = pg::DefaultCommandControls(kTestCmdCtl, {}, {});
    return default_cmd_ctls;
}

DefaultCommandControlScope::DefaultCommandControlScope(storages::postgres::CommandControl default_cmd_ctl)
    : old_cmd_ctl_(GetTestCmdCtls().GetDefaultCmdCtl())
{
    GetTestCmdCtls().UpdateDefaultCmdCtl(default_cmd_ctl);
}

DefaultCommandControlScope::~DefaultCommandControlScope() { GetTestCmdCtls().UpdateDefaultCmdCtl(old_cmd_ctl_); }

engine::Deadline MakeDeadline() { return engine::Deadline::FromDuration(kTestCmdCtl.network_timeout_ms); }

PostgreSQLBase::PostgreSQLBase() = default;

PostgreSQLBase::~PostgreSQLBase() = default;

pg::Dsn PostgreSQLBase::GetDsnFromEnv() {
    auto dsn_list = GetDsnListFromEnv();
    return std::move(dsn_list[0]);
}

pg::DsnList PostgreSQLBase::GetDsnListFromEnv() {
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    auto* conn_list_env = std::getenv(kPostgresDsn);
    if (!conn_list_env) {
        return {pg::Dsn{"postgresql://"}};
    }

    std::vector<std::string> conn_list;
    boost::split(conn_list, conn_list_env, [](char c) { return c == ';'; }, boost::token_compress_on);

    pg::DsnList dsn_list;
    for (auto conn : conn_list) {
        dsn_list.insert(dsn_list.end(), pg::Dsn{std::move(conn)});
    }
    return dsn_list;
}

pg::Dsn PostgreSQLBase::GetUnavailableDsn() { return pg::Dsn{"postgresql://testsuite@localhost:2345/postgres"}; }

storages::postgres::detail::ConnectionPtr PostgreSQLBase::MakeConnection(
    const storages::postgres::Dsn& dsn,
    engine::TaskProcessor& task_processor,
    storages::postgres::ConnectionSettings settings
) {
    std::unique_ptr<pg::detail::Connection> conn;

    try {
        conn = pg::detail::Connection::Connect(
            dsn,
            nullptr,
            task_processor,
            GetTaskStorage(),
            kConnectionId,
            settings,
            GetTestCmdCtls(),
            {},
            {},
            engine::SemaphoreLock{},
            std::make_shared<utils::statistics::MetricsStorage>()
        );
    } catch (const storages::postgres::Error& ex) {
        ADD_FAILURE() << ex.what();
    }

    if (!conn) {
        // Make sure that we signal a fatal failure so that the test body does not
        // run. Otherwise, it may crash.
        [&] { FAIL() << "Failed to connect to DSN"; }();
    }

    pg::detail::ConnectionPtr conn_ptr{std::move(conn)};
    if (conn_ptr) {
        CheckConnection(conn_ptr);
    }
    return conn_ptr;
}

void PostgreSQLBase::CheckConnection(const pg::detail::ConnectionPtr& conn) {
    ASSERT_TRUE(conn) << "Expected non-empty connection pointer";

    ASSERT_TRUE(conn->IsConnected()) << "Connection to PostgreSQL is established";
    ASSERT_TRUE(conn->IsIdle()) << "Connection to PosgreSQL is idle after connection";
    ASSERT_FALSE(conn->IsInTransaction()
    ) << "Connection to PostgreSQL is "
         "not in a transaction after "
         "connection";
}

void PostgreSQLBase::FinalizeConnection(pg::detail::ConnectionPtr conn) {
    UEXPECT_NO_THROW(conn->Close()) << "Successfully close connection";
    EXPECT_FALSE(conn->IsConnected()) << "Connection is not connected";
    EXPECT_FALSE(conn->IsIdle()) << "Connection is not idle";
    EXPECT_FALSE(conn->IsInTransaction()) << "Connection to PostgreSQL is not in a transaction after closing";
}

engine::TaskProcessor& PostgreSQLBase::GetTaskProcessor() { return engine::current_task::GetTaskProcessor(); }

concurrent::BackgroundTaskStorageCore& PostgreSQLBase::GetTaskStorage() {
    static concurrent::BackgroundTaskStorageCore bts;
    return bts;
}

PostgreConnectionBaseFixture::PostgreConnectionBaseFixture() = default;

PostgreConnectionBaseFixture::~PostgreConnectionBaseFixture() {
    // force connection cleanup to avoid leaving detached tasks behind
    engine::AsyncNoTracing(GetTaskProcessor(), [] {}).Wait();
}

storages::postgres::detail::ConnectionPtr PostgreConnectionBaseFixture::MakeConn(
    storages::postgres::ConnectionSettings settings
) {
    return MakeConnection(GetDsnFromEnv(), GetTaskProcessor(), std::move(settings));
}

PostgreConnection::PostgreConnection()
    : conn_(std::unique_ptr<storages::postgres::detail::Connection>{})
{
    const auto& [connection_settings, connection_mode] = GetParam();

    if (connection_mode == ConnectionMode::kChaosProxy) {
        chaos_proxy_ = std::make_unique<
            PostgresChaosProxy>(GetTaskProcessor(), GetPostgresHost(GetDsnFromEnv()), GetPostgresPort(GetDsnFromEnv()));
        conn_ = MakeConnection(
            MakeProxiedDsn(GetDsnFromEnv(), chaos_proxy_->GetPort()),
            GetTaskProcessor(),
            connection_settings
        );
    } else {
        conn_ = MakeConnection(GetDsnFromEnv(), GetTaskProcessor(), connection_settings);
    }
}

PostgreConnection::~PostgreConnection() = default;

storages::postgres::detail::ConnectionPtr& PostgreConnection::GetConn() { return conn_; }

INSTANTIATE_UTEST_SUITE_P(
    ConnectionSettings,
    PostgreConnection,
    ::testing::Combine(
        ::testing::Values(kCachePreparedStatements, kPipelineEnabled, kOmitDescribeAndPipelineEnabled),
        ::testing::Values(ConnectionMode::kDirect, ConnectionMode::kChaosProxy)
    ),
    [](const testing::TestParamInfo<PostgreConnection::ParamType>& info) {
        std::string name{};

        const auto& connection_params = std::get<0>(info.param);
        const auto connection_mode = std::get<1>(info.param);

        if (connection_params.pipeline_mode == pg::PipelineMode::kEnabled) {
            name = "PipelineEnabled";
        } else {
            name = "PipelineDisabled";
        }

        if (connection_params.omit_describe_mode == pg::OmitDescribeInExecuteMode::kEnabled) {
            name.append("_DontSendDescribe");
        } else {
            name.append("_SendDescribe");
        }

        if (connection_mode == ConnectionMode::kChaosProxy) {
            name.append("_ViaChaosProxy");
        }

        return name;
    }
);

USERVER_NAMESPACE_END
