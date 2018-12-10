#include <gtest/gtest.h>

#include <logging/log.hpp>
#include <utest/utest.hpp>

#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>

#include <libpq-fe.h>

namespace {

namespace pg = storages::postgres;

class Fail : public ::testing::TestWithParam<std::string> {};
TEST_P(Fail, InvalidDSN) {
  EXPECT_THROW(pg::SplitByHost(GetParam()), pg::InvalidDSN);
}

INSTANTIATE_TEST_CASE_P(
    PostgreDSN, Fail,
    ::testing::Values("postgresql//",
                      "host=localhost foo=bar"  // unknown parameter
                      ),
    /**/);

struct TestData {
  std::string original_dsn;
  std::size_t dsn_params_count;
  std::string host_and_port;
  std::string dbname;
  std::string host;
};

class Split : public ::testing::TestWithParam<TestData> {};

TEST_P(Split, ByHost) {
  const auto& param = GetParam();

  std::vector<std::string> split_dsn;
  EXPECT_NO_THROW(split_dsn = pg::SplitByHost(param.original_dsn));
  EXPECT_EQ(split_dsn.size(), param.dsn_params_count);
}

TEST_P(Split, FirstHostAndPort) {
  const auto& param = GetParam();

  std::string host_and_port;
  EXPECT_NO_THROW(host_and_port =
                      pg::FirstHostAndPortFromDsn(param.original_dsn));
  EXPECT_EQ(host_and_port, param.host_and_port);
}

TEST_P(Split, FirstDbName) {
  const auto& param = GetParam();

  std::string dbname;
  EXPECT_NO_THROW(dbname = pg::FirstDbNameFromDsn(param.original_dsn));
  EXPECT_EQ(dbname, param.dbname);
}

TEST_P(Split, FirstHost) {
  const auto& param = GetParam();

  std::string host;
  EXPECT_NO_THROW(host = pg::FirstHostNameFromDsn(param.original_dsn));
  EXPECT_EQ(host, param.host);
}

INSTANTIATE_TEST_CASE_P(
    PostgreDSN, Split,
    ::testing::Values(
        TestData{"", 1, "localhost:5432", "", "localhost"},
        TestData{"host=localhost port=5432 dbname=mydb connect_timeout=10", 1,
                 "localhost:5432", "mydb", "localhost"},
        TestData{"host=localhost,host1 port=5432,5433 dbname=mydb "
                 "connect_timeout=10",
                 2, "localhost:5432", "mydb", "localhost"},
        TestData{
            "host=localhost,host1 port=5432 dbname=mydb connect_timeout=10", 2,
            "localhost:5432", "mydb", "localhost"},
        // URIs
        TestData{"postgresql://", 1, "localhost:5432", "", "localhost"},
        TestData{"postgresql://localhost", 1, "localhost:5432", "",
                 "localhost"},
        TestData{"postgresql://localhost:5433", 1, "localhost:5433", "",
                 "localhost"},
        TestData{"postgresql://localhost/mydb", 1, "localhost:5432", "mydb",
                 "localhost"},
        TestData{"postgresql://user@localhost", 1, "localhost:5432", "",
                 "localhost"},
        TestData{"postgresql://user:secret@localhost", 1, "localhost:5432", "",
                 "localhost"},
        TestData{"postgresql://other@localhost/"
                 "otherdb?connect_timeout=10&application_name=myapp",
                 1, "localhost:5432", "otherdb", "localhost"},
        // multi-host uri-like dsn is introduced in PostgreSQL 10.
        TestData{"postgresql://host1:123,host2:456/"
                 "somedb?application_name=myapp",
                 2, "host1:123", "somedb", "host1"},
        // target_session_attrs is introduced in PostgreSQL 10.
        TestData{"postgresql://host1:123,host2:456/"
                 "somedb?target_session_attrs=any&application_name=myapp",
                 2, "host1:123", "somedb", "host1"},
        TestData{"postgresql:///mydb?host=localhost&port=5433", 1,
                 "localhost:5433", "mydb", "localhost"},
        // TODO Fix IPv6 representation. Should be:
        // [2001:db8::1234]:5432
        TestData{"postgresql://[2001:db8::1234]/database", 1,
                 "2001:db8::1234:5432", "database", "2001:db8::1234"},
        TestData{"postgresql:///dbname?host=/var/lib/postgresql", 1,
                 "/var/lib/postgresql:5432", "dbname", "/var/lib/postgresql"},
        TestData{"postgresql://%2Fvar%2Flib%2Fpostgresql/dbname", 1,
                 "/var/lib/postgresql:5432", "dbname", "/var/lib/postgresql"}),
    /**/);

TEST(PostgreDSN, EscapeHostName) {
  EXPECT_EQ(pg::EscapeHostName("host-name.with.numbers130.dots.and-dashes"),
            "host_name_with_numbers130_dots_and_dashes");
}

}  // namespace
