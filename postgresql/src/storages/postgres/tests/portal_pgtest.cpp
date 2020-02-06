#include <storages/postgres/tests/util_pgtest.hpp>

#include <gtest/gtest.h>

#include <storages/postgres/detail/connection.hpp>
#include <storages/postgres/portal.hpp>

namespace pg = storages::postgres;

namespace {

const std::string kGetPostgresTypesSQL = R"~(
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
  left join pg_catalog.pg_namespace n on n.oid = t.typnamespace
order by t.oid)~";

const std::string kPortalName = "";

POSTGRE_TEST_P(PortalLowLevelBindExec) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

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

POSTGRE_TEST_P(PortalClassBindExec) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

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

POSTGRE_TEST_P(NamedPortalClassBindExec) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

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

POSTGRE_TEST_P(PortalCreateFromTrx) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

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

POSTGRE_TEST_P(PortalFetchAll) {
  ASSERT_TRUE(conn.get()) << "Expected non-empty connection pointer";

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

}  // namespace
