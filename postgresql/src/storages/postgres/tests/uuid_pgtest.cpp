#include <userver/storages/postgres/io/uuid.hpp>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <storages/postgres/tests/test_buffers.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/parameter_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace pg = storages::postgres;

UTEST_P(PostgreConnection, UuidRoundtrip) {
  CheckConnection(GetConn());
  boost::uuids::uuid expected = boost::uuids::random_generator{}();
  ASSERT_FALSE(expected.is_nil());

  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(res = GetConn()->Execute("select $1, $1::text", expected));

  boost::uuids::uuid received{};
  std::string string_rep;
  UEXPECT_NO_THROW(res[0].To(received, string_rep));
  EXPECT_EQ(expected, received);
  EXPECT_EQ(to_string(expected), string_rep);
}

UTEST_P(PostgreConnection, UuidStored) {
  CheckConnection(GetConn());
  boost::uuids::uuid expected = boost::uuids::random_generator{}();

  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(res = GetConn()->Execute(
                       "select $1", pg::ParameterStore{}.PushBack(expected)));
  EXPECT_EQ(expected, res[0][0].As<boost::uuids::uuid>());
}

}  // namespace

USERVER_NAMESPACE_END
