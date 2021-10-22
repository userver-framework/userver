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

UTEST_F(PostgreConnection, JsonSelect) {
  CheckConnection(conn);

  pg::ResultSet res{nullptr};
  formats::json::Value json;
  auto expected = formats::json::FromString(kJsonText);

  EXPECT_NO_THROW(res = conn->Execute("select '" + kJsonText + "'"));
  EXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);

  EXPECT_NO_THROW(res = conn->Execute("select '" + kJsonText + "'::json"));
  EXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);

  EXPECT_NO_THROW(res = conn->Execute("select '" + kJsonText + "'::jsonb"));
  EXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);
}

UTEST_F(PostgreConnection, JsonRoundtrip) {
  CheckConnection(conn);

  pg::ResultSet res{nullptr};
  formats::json::Value json;
  auto expected = formats::json::FromString(kJsonText);

  EXPECT_NO_THROW(res = conn->Execute("select $1", expected));
  EXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);

  EXPECT_NO_THROW(res = conn->Execute("select $1::json", expected));
  EXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);

  EXPECT_NO_THROW(res = conn->Execute("select $1::jsonb", expected));
  EXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);
}

UTEST_F(PostgreConnection, JsonRoundtripPlain) {
  CheckConnection(conn);

  pg::ResultSet res{nullptr};
  formats::json::Value json;
  pg::PlainJson expected{formats::json::FromString(kJsonText)};

  EXPECT_NO_THROW(res = conn->Execute("select $1", expected));
  EXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);

  EXPECT_NO_THROW(res = conn->Execute("select $1::json", expected));
  EXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);

  EXPECT_NO_THROW(res = conn->Execute("select $1::jsonb", expected));
  EXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);
}

UTEST_F(PostgreConnection, JsonStored) {
  CheckConnection(conn);

  pg::ResultSet res{nullptr};
  formats::json::Value json;
  auto expected = formats::json::FromString(kJsonText);

  EXPECT_NO_THROW(
      res = conn->Execute("select $1, $2",
                          pg::ParameterStore{}.PushBack(expected).PushBack(
                              pg::PlainJson{expected})));
  EXPECT_NO_THROW(res[0][0].To(json));
  EXPECT_EQ(expected, json);
  EXPECT_NO_THROW(res[0][1].To(json));
  EXPECT_EQ(expected, json);
}

}  // namespace

USERVER_NAMESPACE_END
