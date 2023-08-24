#include <storages/postgres/tests/util_pgtest.hpp>

#include <storages/postgres/detail/connection.hpp>
#include <userver/storages/postgres/io/pg_types.hpp>
#include <userver/storages/postgres/parameter_store.hpp>
#include <userver/storages/postgres/portal.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;

namespace {

const std::string kGetPostgresTypesSQLPrefix = R"~(
select  t.oid,
        n.nspname,
        t.typname,
        t.typlen,
        t.typtype,
        t.typcategory,
        t.typdelim,
        t.typrelid,
        t.typelem,
        t.typarray,
        t.typbasetype,
        t.typnotnull
from pg_catalog.pg_type t
  left join pg_catalog.pg_namespace n on n.oid = t.typnamespace)~";

const std::string kGetPostgresTypesSQL =
    kGetPostgresTypesSQLPrefix + "\norder by t.oid";

// NOLINTNEXTLINE(readability-redundant-string-init)
const std::string kPortalName = "";

UTEST_P(PostgreConnection, PortalLowLevelBindExec) {
  CheckConnection(GetConn());

  EXPECT_ANY_THROW(
      GetConn()->PortalBind(kGetPostgresTypesSQL, kPortalName, {}, {}))
      << "Attempt to bind a portal outside of transaction block should throw "
         "an exception";

  UEXPECT_NO_THROW(GetConn()->Begin({}, pg::detail::SteadyClock::now()));

  auto cnt = GetConn()->Execute("select count(*) from pg_catalog.pg_type t");
  auto expectedCount = cnt.Front().As<pg::Bigint>();
  EXPECT_LT(0, expectedCount);

  pg::detail::Connection::StatementId stmt_id;
  UEXPECT_NO_THROW(stmt_id = GetConn()->PortalBind(kGetPostgresTypesSQL,
                                                   kPortalName, {}, {}));

  // Select some number that is less than actual size of the table
  std::size_t chunkSize = expectedCount / 5;
  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(
      res = GetConn()->PortalExecute(stmt_id, kPortalName, chunkSize, {}));
  EXPECT_EQ(chunkSize, res.Size());
  auto fetched = res.Size();
  // Now fetch other chunks
  while (res.Size() == chunkSize) {
    UEXPECT_NO_THROW(
        res = GetConn()->PortalExecute(stmt_id, kPortalName, chunkSize, {}));
    fetched += res.Size();
  }

  EXPECT_EQ(expectedCount, fetched);

  UEXPECT_NO_THROW(GetConn()->Commit());
}

UTEST_P(PostgreConnection, PortalClassBindExec) {
  CheckConnection(GetConn());

  EXPECT_ANY_THROW(pg::Portal(GetConn().get(), kGetPostgresTypesSQL, {}))
      << "Attempt to bind a portal outside of transaction block should throw "
         "an exception";

  UEXPECT_NO_THROW(GetConn()->Begin({}, pg::detail::SteadyClock::now()));

  auto cnt = GetConn()->Execute("select count(*) from pg_catalog.pg_type t");
  auto expectedCount = cnt.Front().As<pg::Bigint>();

  pg::Portal portal{nullptr, "", {}};
  UEXPECT_NO_THROW(portal =
                       pg::Portal(GetConn().get(), kGetPostgresTypesSQL, {}));

  // Select some number that is less than actual size of the table
  std::size_t chunkSize = expectedCount / 5;
  while (!portal.Done()) {
    UEXPECT_NO_THROW(portal.Fetch(chunkSize));
  }
  EXPECT_EQ(expectedCount, portal.FetchedSoFar());

  EXPECT_ANY_THROW(portal.Fetch(chunkSize));

  UEXPECT_NO_THROW(GetConn()->Commit());
}

UTEST_P(PostgreConnection, NamedPortalClassBindExec) {
  CheckConnection(GetConn());

  EXPECT_ANY_THROW(pg::Portal(GetConn().get(), kGetPostgresTypesSQL, {}))
      << "Attempt to bind a portal outside of transaction block should throw "
         "an exception";

  UEXPECT_NO_THROW(GetConn()->Begin({}, pg::detail::SteadyClock::now()));

  auto cnt = GetConn()->Execute("select count(*) from pg_catalog.pg_type t");
  auto expectedCount = cnt.Front().As<pg::Bigint>();

  pg::Portal portal{nullptr, "", {}};
  UEXPECT_NO_THROW(portal = pg::Portal(GetConn().get(),
                                       pg::PortalName{"TheOrangePortal"},
                                       kGetPostgresTypesSQL, {}));

  // Select some number that is less than actual size of the table
  std::size_t chunkSize = expectedCount / 5;
  while (!portal.Done()) {
    UEXPECT_NO_THROW(portal.Fetch(chunkSize));
  }
  EXPECT_EQ(expectedCount, portal.FetchedSoFar());

  EXPECT_ANY_THROW(portal.Fetch(chunkSize));

  UEXPECT_NO_THROW(GetConn()->Commit());
}

UTEST_P(PostgreConnection, PortalCreateFromTrx) {
  CheckConnection(GetConn());

  pg::Transaction trx{std::move(GetConn()), pg::TransactionOptions{}};
  auto cnt = trx.Execute("select count(*) from pg_catalog.pg_type t");
  auto expectedCount = cnt.Front().As<pg::Bigint>();

  pg::Portal portal{nullptr, "", {}};
  UEXPECT_NO_THROW(portal = trx.MakePortal(kGetPostgresTypesSQL));
  // Select some number that is less than actual size of the table
  std::size_t chunkSize = expectedCount / 5;
  while (!portal.Done()) {
    UEXPECT_NO_THROW(portal.Fetch(chunkSize));
  }
  EXPECT_EQ(expectedCount, portal.FetchedSoFar());

  EXPECT_ANY_THROW(portal.Fetch(chunkSize));
  UEXPECT_NO_THROW(trx.Commit());
}

UTEST_P(PostgreConnection, PortalFetchAll) {
  CheckConnection(GetConn());

  pg::Transaction trx{std::move(GetConn()), pg::TransactionOptions{}};
  auto cnt = trx.Execute("select count(*) from pg_catalog.pg_type t");
  auto expectedCount = cnt.Front().As<pg::Bigint>();

  pg::Portal portal{nullptr, "", {}};
  UEXPECT_NO_THROW(portal = trx.MakePortal(kGetPostgresTypesSQL));
  // Fetch all
  portal.Fetch(0);
  EXPECT_EQ(expectedCount, portal.FetchedSoFar());
  EXPECT_TRUE(portal.Done());

  EXPECT_ANY_THROW(portal.Fetch(0));
  UEXPECT_NO_THROW(trx.Commit());
}

UTEST_P(PostgreConnection, PortalStoredParams) {
  CheckConnection(GetConn());

  pg::Transaction trx{std::move(GetConn()), pg::TransactionOptions{}};
  pg::Portal portal{nullptr, "", {}};
  UEXPECT_NO_THROW(portal = trx.MakePortal(
                       kGetPostgresTypesSQLPrefix + " where t.oid = $1",
                       utils::UnderlyingValue(pg::io::PredefinedOids::kInt8)));
  portal.Fetch(0);
  EXPECT_EQ(1, portal.FetchedSoFar());
  EXPECT_TRUE(portal.Done());

  EXPECT_ANY_THROW(portal.Fetch(0));
  UEXPECT_NO_THROW(trx.Commit());
}

UTEST_P(PostgreConnection, PortalInterleave) {
  constexpr int kIterations = 10;
  constexpr char kQuery[] = "SELECT generate_series(1, $1)";

  CheckConnection(GetConn());
  EXPECT_NO_THROW(
      GetConn()->Execute(kQuery, 1));  // populating the statements cache

  pg::Transaction trx{std::move(GetConn())};
  auto portal = trx.MakePortal(kQuery, kIterations);

  for (int i = 1; i <= kIterations; ++i) {
    auto result = portal.Fetch(1);
    EXPECT_EQ(portal.FetchedSoFar(), i);
    // TODO: TAXICOMMON-4505
    // EXPECT_EQ(portal.Done(), i == kIterations);
    EXPECT_FALSE(portal.Done());
    ASSERT_EQ(result.Size(), 1);
    ASSERT_EQ(result[0].Size(), 1);
    EXPECT_EQ(result[0][0].As<int>(), i);

    result = trx.Execute("SELECT 1");
    ASSERT_EQ(result.Size(), 1);
    ASSERT_EQ(result[0].Size(), 1);
    EXPECT_EQ(result[0][0].As<int>(), 1);
  }

  auto result = portal.Fetch(0);
  EXPECT_TRUE(result.IsEmpty());
  EXPECT_TRUE(portal.Done());
  EXPECT_EQ(portal.FetchedSoFar(), kIterations);
}

namespace {
void PortalTraverse(pg::Portal&& portal, int iterations) {
  for (int i = 1; i <= iterations; ++i) {
    auto result = portal.Fetch(1);
    EXPECT_EQ(portal.FetchedSoFar(), i);
    EXPECT_FALSE(portal.Done());
    ASSERT_EQ(result.Size(), 1);
  }
  auto result = portal.Fetch(0);
  EXPECT_TRUE(result.IsEmpty());
  EXPECT_TRUE(portal.Done());
  EXPECT_EQ(portal.FetchedSoFar(), iterations);
}
}  // namespace

UTEST_P(PostgreConnection, PortalCommandControl) {
  constexpr int kIterations = 10;
  constexpr char kQuery[] = "SELECT generate_series(1, $1)";

  CheckConnection(GetConn());

  // using non-default timings in CommandControl
  pg::Transaction trx{std::move(GetConn())};
  PortalTraverse(
      trx.MakePortal(pg::CommandControl{std::chrono::milliseconds{443},
                                        std::chrono::milliseconds{242}},
                     kQuery, kIterations),
      kIterations);

  // this time the statement should already be in the cache and changing the
  // timings once again
  PortalTraverse(
      trx.MakePortal(pg::CommandControl{std::chrono::milliseconds{444},
                                        std::chrono::milliseconds{243}},
                     kQuery, kIterations),
      kIterations);
}

UTEST_P(PostgreConnection, PortalParallel) {
  constexpr int kIterations = 10;

  CheckConnection(GetConn());

  pg::Transaction trx{std::move(GetConn())};
  auto first = trx.MakePortal("SELECT generate_series(1, $1)", kIterations);
  auto second =
      trx.MakePortal("SELECT generate_series(10, $1, 10)", kIterations * 10);

  for (int i = 1; i <= kIterations; ++i) {
    auto result = first.Fetch(1);
    EXPECT_EQ(first.FetchedSoFar(), i);
    EXPECT_FALSE(first.Done());
    ASSERT_EQ(result.Size(), 1);
    ASSERT_EQ(result[0].Size(), 1);
    EXPECT_EQ(result[0][0].As<int>(), i);

    result = second.Fetch(1);
    EXPECT_EQ(second.FetchedSoFar(), i);
    EXPECT_FALSE(second.Done());
    ASSERT_EQ(result.Size(), 1);
    ASSERT_EQ(result[0].Size(), 1);
    EXPECT_EQ(result[0][0].As<int>(), i * 10);
  }

  auto result = first.Fetch(0);
  EXPECT_TRUE(result.IsEmpty());
  EXPECT_TRUE(first.Done());
  EXPECT_EQ(first.FetchedSoFar(), kIterations);

  result = second.Fetch(0);
  EXPECT_TRUE(result.IsEmpty());
  EXPECT_TRUE(second.Done());
  EXPECT_EQ(second.FetchedSoFar(), kIterations);
}

}  // namespace

USERVER_NAMESPACE_END
