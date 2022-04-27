#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/io/json_types.hpp>
#include <userver/storages/postgres/parameter_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;
namespace tt = io::traits;

namespace {

std::string kJsonText = R"~({
  "zulu" : 3.14,
  "foo" : "bar",
  "baz" : [1, 2, 3]
})~";

UTEST_P(PostgreConnection, JsonSelect) {
  CheckConnection(conn);

  pg::ResultSet res{nullptr};
  formats::json::Value json;
  auto expected = formats::json::FromString(kJsonText);

  UEXPECT_NO_THROW(res = conn->Execute("select '" + kJsonText + "'"));
  UEXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);

  UEXPECT_NO_THROW(res = conn->Execute("select '" + kJsonText + "'::json"));
  UEXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);

  UEXPECT_NO_THROW(res = conn->Execute("select '" + kJsonText + "'::jsonb"));
  UEXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);
}

UTEST_P(PostgreConnection, JsonRoundtrip) {
  CheckConnection(conn);

  pg::ResultSet res{nullptr};
  formats::json::Value json;
  auto expected = formats::json::FromString(kJsonText);

  UEXPECT_NO_THROW(res = conn->Execute("select $1", expected));
  UEXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);

  UEXPECT_NO_THROW(res = conn->Execute("select $1::json", expected));
  UEXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);

  UEXPECT_NO_THROW(res = conn->Execute("select $1::jsonb", expected));
  UEXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);
}

UTEST_P(PostgreConnection, JsonRoundtripPlain) {
  CheckConnection(conn);

  pg::ResultSet res{nullptr};
  formats::json::Value json;
  pg::PlainJson expected{formats::json::FromString(kJsonText)};

  UEXPECT_NO_THROW(res = conn->Execute("select $1", expected));
  UEXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);

  UEXPECT_NO_THROW(res = conn->Execute("select $1::json", expected));
  UEXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);

  UEXPECT_NO_THROW(res = conn->Execute("select $1::jsonb", expected));
  UEXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);
}

UTEST_P(PostgreConnection, JsonStored) {
  CheckConnection(conn);

  pg::ResultSet res{nullptr};
  formats::json::Value json;
  auto expected = formats::json::FromString(kJsonText);

  UEXPECT_NO_THROW(
      res = conn->Execute("select $1, $2",
                          pg::ParameterStore{}.PushBack(expected).PushBack(
                              pg::PlainJson{expected})));
  UEXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);
  UEXPECT_NO_THROW(res[0][1].To(json));
  EXPECT_EQ(expected, json);
}

}  // namespace

USERVER_NAMESPACE_END
