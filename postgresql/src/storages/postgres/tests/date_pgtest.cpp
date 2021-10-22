#include <gtest/gtest.h>

#include <chrono>

#include <storages/postgres/tests/test_buffers.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/io/date.hpp>
#include <userver/storages/postgres/io/user_types.hpp>
#include <userver/storages/postgres/parameter_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;

namespace {

const pg::UserTypes types;

}  // namespace

namespace utils::datetime {

static void PrintTo(const Date& date, std::ostream* os) {
  *os << date.GetSysDays().time_since_epoch().count();
}

}  // namespace utils::datetime

TEST(PostgreIO, Date) {
  const pg::Date today = std::chrono::time_point_cast<pg::Date::Days>(
      std::chrono::system_clock::now());
  pg::test::Buffer buffer;
  EXPECT_NO_THROW(io::WriteBuffer(types, buffer, today));
  auto fb = pg::test::MakeFieldBuffer(buffer);
  pg::Date tgt;
  EXPECT_NO_THROW(io::ReadBuffer(fb, tgt));
  EXPECT_EQ(today, tgt);
}

UTEST_F(PostgreConnection, Date) {
  CheckConnection(conn);

  const pg::Date today = std::chrono::time_point_cast<pg::Date::Days>(
      std::chrono::system_clock::now());

  pg::ResultSet res{nullptr};
  EXPECT_NO_THROW(res = conn->Execute("select $1", today));
  EXPECT_EQ(today, res[0][0].As<pg::Date>());
}

USERVER_NAMESPACE_END
