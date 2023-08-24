#include <userver/utest/utest.hpp>

#include <userver/logging/log.hpp>

#include <userver/storages/postgres/dsn.hpp>
#include <userver/storages/postgres/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace pg = storages::postgres;

class Fail : public ::testing::TestWithParam<pg::Dsn> {};

TEST_P(Fail, InvalidDSN) {
  UEXPECT_THROW(pg::SplitByHost(GetParam()), pg::InvalidDSN);
}

INSTANTIATE_TEST_SUITE_P(
    PostgreDSN, Fail,
    ::testing::Values("postgresql//",
                      "host=localhost foo=bar"  // unknown parameter
                      ));

struct TestData {
  pg::Dsn original_dsn;
  std::size_t dsn_params_count;
  std::string host;
  std::string port;
  std::string dbname;
};

void PrintTo(const TestData& td, std::ostream* os) {
  *os << "TestData{original_dsn=`" << td.original_dsn.GetUnderlying()
      << "`; dsn_params_count=" << td.dsn_params_count << "; host=`" << td.host
      << "`; port=`" << td.port << "`; dbname=`" << td.dbname << "`}";
}

class Split : public ::testing::TestWithParam<TestData> {};

TEST_P(Split, ByHost) {
  const auto& param = GetParam();

  pg::DsnList split_dsn;
  UEXPECT_NO_THROW(split_dsn = pg::SplitByHost(param.original_dsn));
  EXPECT_EQ(split_dsn.size(), param.dsn_params_count);
}

TEST_P(Split, Options) {
  const auto& param = GetParam();

  pg::DsnOptions options;
  UEXPECT_NO_THROW(options = pg::OptionsFromDsn(param.original_dsn));
  EXPECT_EQ(options.host, param.host);
  EXPECT_EQ(options.port, param.port);
  EXPECT_EQ(options.dbname, param.dbname);
}

TEST_P(Split, HostPort) {
  const auto& param = GetParam();

  std::string host_port;
  UEXPECT_NO_THROW(host_port = pg::GetHostPort(param.original_dsn));
  if (param.port.empty()) {
    EXPECT_EQ(host_port, param.host);
  } else {
    EXPECT_EQ(host_port, param.host + ':' + param.port);
  }
}

INSTANTIATE_TEST_SUITE_P(
    PostgreDSN, Split,
    ::testing::Values(
        TestData{pg::Dsn{""}, 1, "localhost", "", ""},
        TestData{
            pg::Dsn{"host=localhost port=5432 dbname=mydb connect_timeout=10"},
            1, "localhost", "5432", "mydb"},
        TestData{pg::Dsn{"host=localhost,host1 port=5432,5433 dbname=mydb "
                         "connect_timeout=10"},
                 2, "localhost", "5432", "mydb"},
        TestData{pg::Dsn{"host=localhost,host1 port=5432 dbname=mydb "
                         "connect_timeout=10"},
                 2, "localhost", "5432", "mydb"},
        TestData{pg::Dsn{"host=/tmp/postgres.sock"}, 1, "/tmp/postgres.sock",
                 "", ""},
        // URIs
        TestData{pg::Dsn{"postgresql://"}, 1, "localhost", "", ""},
        TestData{pg::Dsn{"postgresql://localhost"}, 1, "localhost", "", ""},
        TestData{pg::Dsn{"postgresql://localhost:5433"}, 1, "localhost", "5433",
                 ""},
        TestData{pg::Dsn{"postgresql://localhost/mydb"}, 1, "localhost", "",
                 "mydb"},
        TestData{pg::Dsn{"postgresql://user@localhost"}, 1, "localhost", "",
                 ""},
        TestData{pg::Dsn{"postgresql://user:secret@localhost"}, 1, "localhost",
                 "", ""},
        TestData{pg::Dsn{"postgresql://other@localhost/"
                         "otherdb?connect_timeout=10&application_name=myapp"},
                 1, "localhost", "", "otherdb"},
        TestData{pg::Dsn{"postgresql:///mydb?host=myhost&port=5433"}, 1,
                 "myhost", "5433", "mydb"},
        TestData{pg::Dsn{"postgresql://%2Ftmp%2Fpostgres.sock"}, 1,
                 "/tmp/postgres.sock", "", ""},
        // multi-host uri-like dsn is introduced in PostgreSQL 10.
        TestData{pg::Dsn{"postgresql://host1:123,host2:456/"
                         "somedb?application_name=myapp"},
                 2, "host1", "123", "somedb"},
        // target_session_attrs is introduced in PostgreSQL 10.
        TestData{
            pg::Dsn{"postgresql://host1:123,host2:456/"
                    "somedb?target_session_attrs=any&application_name=myapp"},
            2, "host1", "123", "somedb"},
        TestData{pg::Dsn{"postgresql:///mydb?host=localhost&port=5433"}, 1,
                 "localhost", "5433", "mydb"},
        TestData{pg::Dsn{"postgresql://[2001:db8::1234]/database"}, 1,
                 "2001:db8::1234", "", "database"},
        TestData{pg::Dsn{"postgresql:///dbname?host=/var/lib/postgresql"}, 1,
                 "/var/lib/postgresql", "", "dbname"},
        TestData{pg::Dsn{"postgresql://%2Fvar%2Flib%2Fpostgresql/dbname"}, 1,
                 "/var/lib/postgresql", "", "dbname"}));

TEST(PostgreDSN, DsnCutPassword) {
  auto dsn_cut = pg::DsnCutPassword(pg::Dsn{
      "host=127.0.0.1 port=6432 dbname=mydb connect_timeout=10 user=myuser "
      "password=mypass"});
  EXPECT_EQ(dsn_cut.find("password"), dsn_cut.npos);
  EXPECT_EQ(dsn_cut.find("mypass"), dsn_cut.npos);

  pg::DsnOptions options;
  UEXPECT_NO_THROW(options = pg::OptionsFromDsn(pg::Dsn{std::move(dsn_cut)}));
  EXPECT_EQ(options.host, "127.0.0.1");
  EXPECT_EQ(options.port, "6432");
  EXPECT_EQ(options.dbname, "mydb");
}

TEST(PostgreDSN, EscapeHostName) {
  EXPECT_EQ(pg::EscapeHostName("host-name.with.numbers130.dots.and-dashes"),
            "host_name_with_numbers130_dots_and_dashes");
}

TEST(PostgreDSN, QuotedOptions) {
  const std::string dsn = R"( options='-c \'backslash=\\\'')";

  pg::DsnList split_dsn;
  UEXPECT_NO_THROW(split_dsn = pg::SplitByHost(pg::Dsn{dsn}));
  ASSERT_EQ(split_dsn.size(), 1);
  EXPECT_EQ(split_dsn.front().GetUnderlying(), dsn);
}

class Mask : public ::testing::TestWithParam<std::pair<pg::Dsn, std::string>> {
};

TEST_P(Mask, MaskDSN) {
  auto param = GetParam();
  EXPECT_EQ(param.second, pg::DsnMaskPassword(param.first));
}

INSTANTIATE_TEST_SUITE_P(
    PostgreDSN, Mask,
    ::testing::Values(
        std::make_pair(pg::Dsn{""}, ""),
        std::make_pair(pg::Dsn{"host=localhost"}, "host=localhost"),
        std::make_pair(pg::Dsn{"host=localhost password="},
                       "host=localhost password="),
        std::make_pair(pg::Dsn{"host=localhost password=pwd"},
                       "host=localhost password=***"),
        std::make_pair(pg::Dsn{"host=localhost password = pwd"},
                       "host=localhost password = ***"),
        std::make_pair(pg::Dsn{"host=localhost password=p%24wd"},
                       "host=localhost password=***"),
        std::make_pair(pg::Dsn{"host=localhost password='my secret'"},
                       "host=localhost password=***"),
        std::make_pair(pg::Dsn{"host=localhost password='my \\' secret'"},
                       "host=localhost password=***"),
        std::make_pair(pg::Dsn{"host=localhost password = 'my \\' secret'"},
                       "host=localhost password = ***"),
        std::make_pair(pg::Dsn{"host=localhost password=pwd user=user"},
                       "host=localhost password=*** user=user"),
        std::make_pair(pg::Dsn{"postgresql://localhost"},
                       "postgresql://localhost"),
        std::make_pair(pg::Dsn{"postgresql://user@localhost"},
                       "postgresql://user@localhost"),
        std::make_pair(pg::Dsn{"postgresql://@localhost"},
                       "postgresql://@localhost"),  // not actually a valid
                                                    // connection string
        std::make_pair(pg::Dsn{"postgresql://:pwd@localhost"},
                       "postgresql://:***@localhost"),  // not actually a valid
                                                        // connection string
        std::make_pair(pg::Dsn{"postgresql://:@localhost"},
                       "postgresql://:@localhost"),  // not actually a valid
                                                     // connection string
        std::make_pair(pg::Dsn{"postgresql://user:pwd@localhost"},
                       "postgresql://user:***@localhost"),
        std::make_pair(pg::Dsn{"postgresql://user:p%24wd@localhost"},
                       "postgresql://user:***@localhost"),
        std::make_pair(
            pg::Dsn{"postgresql://user@localhost/somedb?password=pwd"},
            "postgresql://user@localhost/somedb?password=***"),
        std::make_pair(
            pg::Dsn{"postgresql://user@localhost/somedb?password=p%24wd"},
            "postgresql://user@localhost/somedb?password=***"),
        std::make_pair(pg::Dsn{"postgresql://user@localhost/"
                               "somedb?password=pwd&application_name=myapp"},
                       "postgresql://user@localhost/"
                       "somedb?password=***&application_name=myapp"),
        std::make_pair(
            pg::Dsn{"postgresql://user@localhost/"
                    "somedb?password=pwd&application_name=myapp&password=pwd"},
            "postgresql://user@localhost/"
            "somedb?password=***&application_name=myapp&password=***"),
        std::make_pair(
            pg::Dsn{"postgresql://user:pwd@localhost/"
                    "somedb?password=pwd&application_name=myapp&password=pwd"},
            "postgresql://user:***@localhost/"
            "somedb?password=***&application_name=myapp&password=***"),
        std::make_pair(pg::Dsn{"postgresql://user@localhost/"
                               "somedb?password=p%24wd&application_name=myapp"},
                       "postgresql://user@localhost/"
                       "somedb?password=***&application_name=myapp")));

}  // namespace

USERVER_NAMESPACE_END
