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
  std::string host;
  std::string port;
  std::string dbname;
};

class Split : public ::testing::TestWithParam<TestData> {};

TEST_P(Split, ByHost) {
  const auto& param = GetParam();

  std::vector<std::string> split_dsn;
  EXPECT_NO_THROW(split_dsn = pg::SplitByHost(param.original_dsn));
  EXPECT_EQ(split_dsn.size(), param.dsn_params_count);
}

TEST_P(Split, Options) {
  const auto& param = GetParam();

  pg::DsnOptions options;
  EXPECT_NO_THROW(options = pg::OptionsFromDsn(param.original_dsn));
  EXPECT_EQ(options.host, param.host);
  EXPECT_EQ(options.port, param.port);
  EXPECT_EQ(options.dbname, param.dbname);
}

INSTANTIATE_TEST_CASE_P(
    PostgreDSN, Split,
    ::testing::Values(
        TestData{"", 1, "localhost", "5432", ""},
        TestData{"host=localhost port=5432 dbname=mydb connect_timeout=10", 1,
                 "localhost", "5432", "mydb"},
        TestData{"host=localhost,host1 port=5432,5433 dbname=mydb "
                 "connect_timeout=10",
                 2, "localhost", "5432", "mydb"},
        TestData{
            "host=localhost,host1 port=5432 dbname=mydb connect_timeout=10", 2,
            "localhost", "5432", "mydb"},
        // URIs
        TestData{"postgresql://", 1, "localhost", "5432", ""},
        TestData{"postgresql://localhost", 1, "localhost", "5432", ""},
        TestData{"postgresql://localhost:5433", 1, "localhost", "5433", ""},
        TestData{"postgresql://localhost/mydb", 1, "localhost", "5432", "mydb"},
        TestData{"postgresql://user@localhost", 1, "localhost", "5432", ""},
        TestData{"postgresql://user:secret@localhost", 1, "localhost", "5432",
                 ""},
        TestData{"postgresql://other@localhost/"
                 "otherdb?connect_timeout=10&application_name=myapp",
                 1, "localhost", "5432", "otherdb"},
        // multi-host uri-like dsn is introduced in PostgreSQL 10.
        TestData{"postgresql://host1:123,host2:456/"
                 "somedb?application_name=myapp",
                 2, "host1", "123", "somedb"},
        // target_session_attrs is introduced in PostgreSQL 10.
        TestData{"postgresql://host1:123,host2:456/"
                 "somedb?target_session_attrs=any&application_name=myapp",
                 2, "host1", "123", "somedb"},
        TestData{"postgresql:///mydb?host=localhost&port=5433", 1, "localhost",
                 "5433", "mydb"},
        TestData{"postgresql://[2001:db8::1234]/database", 1, "2001:db8::1234",
                 "5432", "database"},
        TestData{"postgresql:///dbname?host=/var/lib/postgresql", 1,
                 "/var/lib/postgresql", "5432", "dbname"},
        TestData{"postgresql://%2Fvar%2Flib%2Fpostgresql/dbname", 1,
                 "/var/lib/postgresql", "5432", "dbname"}),
    /**/);

TEST(PostgreDSN, DsnCutPassword) {
  const auto dsn_cut = pg::DsnCutPassword(
      "host=127.0.0.1 port=6432 dbname=mydb connect_timeout=10 user=myuser "
      "password=mypass");
  EXPECT_EQ(dsn_cut.find("password"), dsn_cut.npos);
  EXPECT_EQ(dsn_cut.find("mypass"), dsn_cut.npos);

  pg::DsnOptions options;
  EXPECT_NO_THROW(options = pg::OptionsFromDsn(dsn_cut));
  EXPECT_EQ(options.host, "127.0.0.1");
  EXPECT_EQ(options.port, "6432");
  EXPECT_EQ(options.dbname, "mydb");
}

TEST(PostgreDSN, EscapeHostName) {
  EXPECT_EQ(pg::EscapeHostName("host-name.with.numbers130.dots.and-dashes"),
            "host_name_with_numbers130_dots_and_dashes");
}

class Mask
    : public ::testing::TestWithParam<std::pair<std::string, std::string>> {};

TEST_P(Mask, MaskDSN) {
  auto param = GetParam();
  EXPECT_EQ(param.second, pg::DsnMaskPassword(param.first));
}

INSTANTIATE_TEST_CASE_P(
    PostgreDSN, Mask,
    ::testing::Values(
        std::make_pair("", ""),
        std::make_pair("host=localhost", "host=localhost"),
        std::make_pair("host=localhost password=", "host=localhost password="),
        std::make_pair("host=localhost password=pwd",
                       "host=localhost password=123456"),
        std::make_pair("host=localhost password = pwd",
                       "host=localhost password = 123456"),
        std::make_pair("host=localhost password=p%24wd",
                       "host=localhost password=123456"),
        std::make_pair("host=localhost password='my secret'",
                       "host=localhost password=123456"),
        std::make_pair("host=localhost password='my \\' secret'",
                       "host=localhost password=123456"),
        std::make_pair("host=localhost password = 'my \\' secret'",
                       "host=localhost password = 123456"),
        std::make_pair("host=localhost password=pwd user=user",
                       "host=localhost password=123456 user=user"),
        std::make_pair("postgresql://localhost", "postgresql://localhost"),
        std::make_pair("postgresql://user@localhost",
                       "postgresql://user@localhost"),
        std::make_pair("postgresql://@localhost",
                       "postgresql://@localhost"),  // not actually a valid
                                                    // connection string
        std::make_pair(
            "postgresql://:pwd@localhost",
            "postgresql://:123456@localhost"),  // not actually a valid
                                                // connection string
        std::make_pair("postgresql://:@localhost",
                       "postgresql://:@localhost"),  // not actually a valid
                                                     // connection string
        std::make_pair("postgresql://user:pwd@localhost",
                       "postgresql://user:123456@localhost"),
        std::make_pair("postgresql://user:p%24wd@localhost",
                       "postgresql://user:123456@localhost"),
        std::make_pair("postgresql://user@localhost/somedb?password=pwd",
                       "postgresql://user@localhost/somedb?password=123456"),
        std::make_pair("postgresql://user@localhost/somedb?password=p%24wd",
                       "postgresql://user@localhost/somedb?password=123456"),
        std::make_pair("postgresql://user@localhost/"
                       "somedb?password=pwd&application_name=myapp",
                       "postgresql://user@localhost/"
                       "somedb?password=123456&application_name=myapp"),
        std::make_pair(
            "postgresql://user@localhost/"
            "somedb?password=pwd&application_name=myapp&password=pwd",
            "postgresql://user@localhost/"
            "somedb?password=123456&application_name=myapp&password=123456"),
        std::make_pair(
            "postgresql://user:pwd@localhost/"
            "somedb?password=pwd&application_name=myapp&password=pwd",
            "postgresql://user:123456@localhost/"
            "somedb?password=123456&application_name=myapp&password=123456"),
        std::make_pair("postgresql://user@localhost/"
                       "somedb?password=p%24wd&application_name=myapp",
                       "postgresql://user@localhost/"
                       "somedb?password=123456&application_name=myapp")),
    /**/);

}  // namespace
