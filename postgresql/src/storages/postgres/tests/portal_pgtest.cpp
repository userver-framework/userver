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

const std::string kPortalName = "";

UTEST_F(PostgreConnection, PortalLowLevelBindExec) {
  CheckConnection(conn);

  EXPECT_ANY_THROW(conn->PortalBind(kGetPostgresTypesSQL, kPortalName, {}, {}))
      << "Attempt to bind a portal outside of transaction block should throw "
         "an exception";

  EXPECT_NO_THROW(conn->Begin({}, pg::detail::SteadyClock::now()));

  auto cnt = conn->Execute("select count(*) from pg_catalog.pg_type t");
  auto expectedCount = cnt.Front().As<pg::Bigint>();
  EXPECT_LT(0, expectedCount);

  pg::detail::Connection::StatementId stmt_id;
  EXPECT_NO_THROW(
      stmt_id = conn->PortalBind(kGetPostgresTypesSQL, kPortalName, {}, {}));

  // Select some number that is less than actual size of the table
  std::size_t chunkSize = expectedCount / 5;
  pg::ResultSet res{nullptr};
  EXPECT_NO_THROW(res =
                      conn->PortalExecute(stmt_id, kPortalName, chunkSize, {}));
  EXPECT_EQ(chunkSize, res.Size());
  auto fetched = res.Size();
  // Now fetch other chunks
  while (res.Size() == chunkSize) {
    EXPECT_NO_THROW(
        res = conn->PortalExecute(stmt_id, kPortalName, chunkSize, {}));
    fetched += res.Size();
  }

  EXPECT_EQ(expectedCount, fetched);

  EXPECT_NO_THROW(conn->Commit());
}

UTEST_F(PostgreConnection, PortalClassBindExec) {
  CheckConnection(conn);

  EXPECT_ANY_THROW(pg::Portal(conn.get(), kGetPostgresTypesSQL, {}))
      << "Attempt to bind a portal outside of transaction block should throw "
         "an exception";

  EXPECT_NO_THROW(conn->Begin({}, pg::detail::SteadyClock::now()));

  auto cnt = conn->Execute("select count(*) from pg_catalog.pg_type t");
  auto expectedCount = cnt.Front().As<pg::Bigint>();

  pg::Portal portal{nullptr, "", {}};
  EXPECT_NO_THROW(portal = pg::Portal(conn.get(), kGetPostgresTypesSQL, {}));

  // Select some number that is less than actual size of the table
  std::size_t chunkSize = expectedCount / 5;
  while (!portal.Done()) {
    EXPECT_NO_THROW(portal.Fetch(chunkSize));
  }
  EXPECT_EQ(expectedCount, portal.FetchedSoFar());

  EXPECT_ANY_THROW(portal.Fetch(chunkSize));

  EXPECT_NO_THROW(conn->Commit());
}

UTEST_F(PostgreConnection, NamedPortalClassBindExec) {
  CheckConnection(conn);

  EXPECT_ANY_THROW(pg::Portal(conn.get(), kGetPostgresTypesSQL, {}))
      << "Attempt to bind a portal outside of transaction block should throw "
         "an exception";

  EXPECT_NO_THROW(conn->Begin({}, pg::detail::SteadyClock::now()));

  auto cnt = conn->Execute("select count(*) from pg_catalog.pg_type t");
  auto expectedCount = cnt.Front().As<pg::Bigint>();

  pg::Portal portal{nullptr, "", {}};
  EXPECT_NO_THROW(portal =
                      pg::Portal(conn.get(), pg::PortalName{"TheOrangePortal"},
                                 kGetPostgresTypesSQL, {}));

  // Select some number that is less than actual size of the table
  std::size_t chunkSize = expectedCount / 5;
  while (!portal.Done()) {
    EXPECT_NO_THROW(portal.Fetch(chunkSize));
  }
  EXPECT_EQ(expectedCount, portal.FetchedSoFar());

  EXPECT_ANY_THROW(portal.Fetch(chunkSize));

  EXPECT_NO_THROW(conn->Commit());
}

UTEST_F(PostgreConnection, PortalCreateFromTrx) {
  CheckConnection(conn);

  pg::Transaction trx{std::move(conn), pg::TransactionOptions{}};
  auto cnt = trx.Execute("select count(*) from pg_catalog.pg_type t");
  auto expectedCount = cnt.Front().As<pg::Bigint>();

  pg::Portal portal{nullptr, "", {}};
  EXPECT_NO_THROW(portal = trx.MakePortal(kGetPostgresTypesSQL));
  // Select some number that is less than actual size of the table
  std::size_t chunkSize = expectedCount / 5;
  while (!portal.Done()) {
    EXPECT_NO_THROW(portal.Fetch(chunkSize));
  }
  EXPECT_EQ(expectedCount, portal.FetchedSoFar());

  EXPECT_ANY_THROW(portal.Fetch(chunkSize));
  EXPECT_NO_THROW(trx.Commit());
}

UTEST_F(PostgreConnection, PortalFetchAll) {
  CheckConnection(conn);

  pg::Transaction trx{std::move(conn), pg::TransactionOptions{}};
  auto cnt = trx.Execute("select count(*) from pg_catalog.pg_type t");
  auto expectedCount = cnt.Front().As<pg::Bigint>();

  pg::Portal portal{nullptr, "", {}};
  EXPECT_NO_THROW(portal = trx.MakePortal(kGetPostgresTypesSQL));
  // Fetch all
  portal.Fetch(0);
  EXPECT_EQ(expectedCount, portal.FetchedSoFar());
  EXPECT_TRUE(portal.Done());

  EXPECT_ANY_THROW(portal.Fetch(0));
  EXPECT_NO_THROW(trx.Commit());
}

UTEST_F(PostgreConnection, PortalStoredParams) {
  CheckConnection(conn);

  pg::Transaction trx{std::move(conn), pg::TransactionOptions{}};
  pg::Portal portal{nullptr, "", {}};
  EXPECT_NO_THROW(
      portal = trx.MakePortal(kGetPostgresTypesSQLPrefix + " where t.oid = $1",
                              pg::io::PredefinedOids::kInt8));
  portal.Fetch(0);
  EXPECT_EQ(1, portal.FetchedSoFar());
  EXPECT_TRUE(portal.Done());

  EXPECT_ANY_THROW(portal.Fetch(0));
  EXPECT_NO_THROW(trx.Commit());
}

UTEST_F(PostgreConnection, PortalInterleave) {
  constexpr int kIterations = 10;

  CheckConnection(conn);

  pg::Transaction trx{std::move(conn)};
  auto portal = trx.MakePortal("SELECT generate_series(1, $1)", kIterations);

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

UTEST_F(PostgreConnection, PortalParallel) {
  constexpr int kIterations = 10;

  CheckConnection(conn);

  pg::Transaction trx{std::move(conn)};
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
