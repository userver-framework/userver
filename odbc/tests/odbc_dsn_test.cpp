#include <userver/utest/utest.hpp>

#include <storages/odbc/dsn.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::odbc::tests {

UTEST(ParseDsn, BasicDsn) {
    const Dsn dsn{
        "DRIVER={PostgreSQL Unicode};"
        "SERVER=localhost;"
        "PORT=5432;"
        "DATABASE=mydb;"
        "UID=user;"
        "PWD=secret;"
    };

    auto opts = ParseDsn(dsn);
    EXPECT_EQ(opts.driver, "PostgreSQL Unicode");
    EXPECT_EQ(opts.server, "localhost");
    EXPECT_EQ(opts.port, "5432");
    EXPECT_EQ(opts.database, "mydb");
    EXPECT_EQ(opts.uid, "user");
}

UTEST(ParseDsn, NoTrailingSemicolon) {
    const Dsn dsn{"SERVER=host;PORT=1234;DATABASE=db"};

    auto opts = ParseDsn(dsn);
    EXPECT_EQ(opts.server, "host");
    EXPECT_EQ(opts.port, "1234");
    EXPECT_EQ(opts.database, "db");
}

UTEST(ParseDsn, CaseInsensitiveKeys) {
    const Dsn dsn{"server=myhost;port=9999;database=testdb;uid=admin"};

    auto opts = ParseDsn(dsn);
    EXPECT_EQ(opts.server, "myhost");
    EXPECT_EQ(opts.port, "9999");
    EXPECT_EQ(opts.database, "testdb");
    EXPECT_EQ(opts.uid, "admin");
}

UTEST(ParseDsn, AlternativeKeyNames) {
    const Dsn dsn{"HOST=althost;DB=altdb;USERNAME=altuser"};

    auto opts = ParseDsn(dsn);
    EXPECT_EQ(opts.server, "althost");
    EXPECT_EQ(opts.database, "altdb");
    EXPECT_EQ(opts.uid, "altuser");
}

UTEST(ParseDsn, EmptyDsn) {
    const Dsn dsn{""};

    auto opts = ParseDsn(dsn);
    EXPECT_TRUE(opts.driver.empty());
    EXPECT_TRUE(opts.server.empty());
    EXPECT_TRUE(opts.port.empty());
    EXPECT_TRUE(opts.database.empty());
    EXPECT_TRUE(opts.uid.empty());
}

UTEST(ParseDsn, BracedDriverValue) {
    const Dsn dsn{"DRIVER={ODBC Driver 17 for SQL Server};SERVER=sqlhost"};

    auto opts = ParseDsn(dsn);
    EXPECT_EQ(opts.driver, "ODBC Driver 17 for SQL Server");
    EXPECT_EQ(opts.server, "sqlhost");
}

UTEST(GetHostPort, ServerAndPort) {
    const Dsn dsn{"SERVER=dbhost;PORT=5433"};
    EXPECT_EQ(GetHostPort(dsn), "dbhost:5433");
}

UTEST(GetHostPort, ServerOnly) {
    const Dsn dsn{"SERVER=dbhost"};
    EXPECT_EQ(GetHostPort(dsn), "dbhost");
}

UTEST(GetHostPort, NoServer) {
    const Dsn dsn{"DATABASE=mydb"};
    EXPECT_TRUE(GetHostPort(dsn).empty());
}

UTEST(DsnCutPassword, RemovesPassword) {
    const Dsn dsn{"SERVER=host;PWD=secret;DATABASE=db"};
    auto result = DsnCutPassword(dsn);
    EXPECT_EQ(result.find("secret"), std::string::npos);
    EXPECT_EQ(result.find("PWD"), std::string::npos);
    EXPECT_NE(result.find("SERVER"), std::string::npos);
    EXPECT_NE(result.find("DATABASE"), std::string::npos);
}

UTEST(DsnCutPassword, NoPassword) {
    const Dsn dsn{"SERVER=host;DATABASE=db"};
    auto result = DsnCutPassword(dsn);
    EXPECT_NE(result.find("SERVER"), std::string::npos);
    EXPECT_NE(result.find("DATABASE"), std::string::npos);
}

UTEST(DsnMaskPassword, MasksPassword) {
    const Dsn dsn{"SERVER=host;PWD=secret;DATABASE=db"};
    auto result = DsnMaskPassword(dsn);
    EXPECT_EQ(result.find("secret"), std::string::npos);
    EXPECT_NE(result.find("***"), std::string::npos);
    EXPECT_NE(result.find("SERVER"), std::string::npos);
}

UTEST(DsnMaskPassword, NoPassword) {
    const Dsn dsn{"SERVER=host;DATABASE=db"};
    auto result = DsnMaskPassword(dsn);
    EXPECT_EQ(result.find("***"), std::string::npos);
}

UTEST(IsIpAddress, IPv4) {
    EXPECT_TRUE(IsIpAddress("192.168.1.1"));
    EXPECT_TRUE(IsIpAddress("10.0.0.1"));
    EXPECT_TRUE(IsIpAddress("127.0.0.1"));
}

UTEST(IsIpAddress, IPv6) {
    EXPECT_TRUE(IsIpAddress("::1"));
    EXPECT_TRUE(IsIpAddress("fe80::1"));
    EXPECT_TRUE(IsIpAddress("2001:db8::1"));
}

UTEST(IsIpAddress, Hostname) {
    EXPECT_FALSE(IsIpAddress("localhost"));
    EXPECT_FALSE(IsIpAddress("db.example.com"));
    EXPECT_FALSE(IsIpAddress("my-server"));
}

UTEST(IsIpAddress, Empty) { EXPECT_FALSE(IsIpAddress("")); }

}  // namespace storages::odbc::tests

USERVER_NAMESPACE_END
