#include <storages/postgres/connlimit_watchdog.hpp>

#include <storages/postgres/detail/cluster_impl.hpp>
#include <userver/hostinfo/blocking/get_hostname.hpp>
#include <userver/utils/from_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

namespace {
constexpr CommandControl kCommandControl{std::chrono::seconds(2),
                                         std::chrono::seconds(2)};
constexpr size_t kTestsuiteConnlimit = 100;
constexpr size_t kReservedConn = 5;

constexpr size_t kMaxStepsWithError = 3;
constexpr size_t kFallbackConnlimit = 20;
}  // namespace

ConnlimitWatchdog::ConnlimitWatchdog(detail::ClusterImpl& cluster,
                                     testsuite::TestsuiteTasks& testsuite_tasks,
                                     std::function<void()> on_new_connlimit)
    : cluster_(cluster),
      connlimit_(0),
      on_new_connlimit_(std::move(on_new_connlimit)),
      testsuite_tasks_(testsuite_tasks) {}

void ConnlimitWatchdog::Start() {
  auto trx = cluster_.Begin({ClusterHostType::kMaster}, {}, kCommandControl);
  trx.Execute(
      "CREATE TABLE IF NOT EXISTS u_clients (hostname TEXT PRIMARY KEY, "
      "updated "
      "TIMESTAMPTZ NOT NULL, max_connections INTEGER NOT NULL)");
  trx.Commit();

  if (testsuite_tasks_.IsEnabled()) {
    connlimit_ = kTestsuiteConnlimit;
    testsuite_tasks_.RegisterTask("connlimit_watchdog_" + cluster_.GetDbName(),
                                  [this] { Step(); });
  } else {
    periodic_.Start("connlimit_watchdog",
                    {std::chrono::seconds(2),
                     {},
                     {USERVER_NAMESPACE::utils::PeriodicTask::Flags::kNow}},
                    [this] { Step(); });
  }
}

void ConnlimitWatchdog::Step() {
  static auto kHostname = hostinfo::blocking::GetRealHostName();
  try {
    auto trx = cluster_.Begin({ClusterHostType::kMaster}, {}, kCommandControl);
    auto max_connections = USERVER_NAMESPACE::utils::FromString<size_t>(
        trx.Execute("SHOW max_connections;").AsSingleRow<std::string>());

    if (max_connections > kReservedConn)
      max_connections -= kReservedConn;
    else
      max_connections = 1;

    trx.Execute(
        "INSERT INTO u_clients (hostname, updated, max_connections) VALUES "
        "($1, "
        "NOW(), $2) ON CONFLICT (hostname) DO UPDATE SET updated = NOW(), "
        "max_connections = $2",
        kHostname, static_cast<int>(GetConnlimit()));
    auto instances = trx.Execute(
                            "SELECT count(*) FROM u_clients WHERE updated >= "
                            "NOW() - make_interval(secs => 15)")
                         .AsSingleRow<int>();
    if (instances == 0) instances = 1;

    auto connlimit = max_connections / instances;
    if (connlimit == 0) connlimit = 1;
    LOG_DEBUG() << "max_connections = " << max_connections
                << ", instances = " << instances
                << ", connlimit = " << connlimit;
    connlimit_ = connlimit;

    trx.Commit();
    steps_with_errors_ = 0;
  } catch (const Error& e) {
    if (++steps_with_errors_ > kMaxStepsWithError) {
      /*
       * Something's wrong with PG server. Try to lower the load by lowering
       * max connection to a small value. Active connections will be gracefully
       * closed. When the server returns the response, we'll get the real
       * connlimit value. The period with "too low max_connections" should be
       * relatively small.
       */
      connlimit_ = kFallbackConnlimit;
    }
  }

  on_new_connlimit_();
}

void ConnlimitWatchdog::Stop() { periodic_.Stop(); }

size_t ConnlimitWatchdog::GetConnlimit() const { return connlimit_.load(); }

}  // namespace storages::postgres

USERVER_NAMESPACE_END
