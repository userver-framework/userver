#include <gtest/gtest.h>

#include <logging/log.hpp>
#include <utest/utest.hpp>

#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>

#include <postgresql/libpq-fe.h>

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

using TestData = std::pair<std::string, std::size_t>;
class Split : public ::testing::TestWithParam<TestData> {};

TEST_P(Split, ByHost) {
  const auto& param = GetParam();

  std::vector<std::string> split_dsn;
  EXPECT_NO_THROW(split_dsn = pg::SplitByHost(param.first));
  EXPECT_EQ(param.second, split_dsn.size());
}

INSTANTIATE_TEST_CASE_P(
    PostgreDSN, Split,
    ::testing::Values(
        std::make_pair("", 1),
        std::make_pair(
            "host=localhost port=5432 dbname=mydb connect_timeout=10", 1),
        std::make_pair("host=localhost,host1 port=5432,5433 dbname=mydb "
                       "connect_timeout=10",
                       2),
        std::make_pair(
            "host=localhost,host1 port=5432 dbname=mydb connect_timeout=10", 2),
        // URIs
        std::make_pair("postgresql://", 1),
        std::make_pair("postgresql://localhost", 1),
        std::make_pair("postgresql://localhost:5433", 1),
        std::make_pair("postgresql://localhost/mydb", 1),
        std::make_pair("postgresql://user@localhost", 1),
        std::make_pair("postgresql://user:secret@localhost", 1),
        std::make_pair("postgresql://other@localhost/"
                       "otherdb?connect_timeout=10&application_name=myapp",
                       1),
        // multi-host uri-like dsn is introduced in PostgreSQL 10.
        // TODO Restore the test as soon as we replace all postgre libs with
        // version 10
        // std::make_pair("postgresql://host1:123,host2:456/"
        //               "somedb?application_name=myapp",
        //               2),
        // target_session_attrs is introduced in PostgreSQL 10.
        // TODO Restore the test as soon as we replace all postgre libs with
        // version 10
        // std::make_pair("postgresql://host1:123,host2:456/"
        //               "somedb?target_session_attrs=any&application_name=myapp",
        //               2),
        std::make_pair("postgresql:///mydb?host=localhost&port=5433", 1),
        std::make_pair("postgresql://[2001:db8::1234]/database", 1),
        std::make_pair("postgresql:///dbname?host=/var/lib/postgresql", 1),
        std::make_pair("postgresql://%2Fvar%2Flib%2Fpostgresql/dbname", 1)),
    /**/);

}  // namespace
