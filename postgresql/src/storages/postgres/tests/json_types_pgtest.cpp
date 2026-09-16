#include <gtest/gtest.h>

#include <storages/postgres/tests/test_buffers.hpp>
#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/formats/json/raw_string.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/field_buffer.hpp>
#include <userver/storages/postgres/io/json_types.hpp>
#include <userver/storages/postgres/parameter_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;

namespace {

const pg::UserTypes kTypes;

std::string kJsonText = R"~({
  "zulu" : 3.14,
  "foo" : "bar",
  "baz" : [1, 2, 3]
})~";

UTEST_P(PostgreConnection, JsonSelect) {
    CheckConnection(GetConn());

    pg::ResultSet res{nullptr};
    formats::json::Value json;
    auto expected = formats::json::FromString(kJsonText);

    UEXPECT_NO_THROW(res = GetConn()->Execute("select '" + kJsonText + "'"));
    UEXPECT_NO_THROW(res[0][0].To(json));
    EXPECT_EQ(expected, json);

    UEXPECT_NO_THROW(res = GetConn()->Execute("select '" + kJsonText + "'::json"));
    UEXPECT_NO_THROW(res[0][0].To(json));
    EXPECT_EQ(expected, json);

    UEXPECT_NO_THROW(res = GetConn()->Execute("select '" + kJsonText + "'::jsonb"));
    UEXPECT_NO_THROW(res[0][0].To(json));
    EXPECT_EQ(expected, json);
}

UTEST_P(PostgreConnection, JsonRoundtrip) {
    CheckConnection(GetConn());

    pg::ResultSet res{nullptr};
    formats::json::Value json;
    auto expected = formats::json::FromString(kJsonText);

    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", expected));
    UEXPECT_NO_THROW(res[0][0].To(json));
    EXPECT_EQ(expected, json);

    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1::json", expected));
    UEXPECT_NO_THROW(res[0][0].To(json));
    EXPECT_EQ(expected, json);

    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1::jsonb", expected));
    UEXPECT_NO_THROW(res[0][0].To(json));
    EXPECT_EQ(expected, json);
}

UTEST_P(PostgreConnection, JsonRoundtripPlain) {
    CheckConnection(GetConn());

    pg::ResultSet res{nullptr};
    formats::json::Value json;
    pg::PlainJson expected{formats::json::FromString(kJsonText)};

    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", expected));
    UEXPECT_NO_THROW(res[0][0].To(json));
    EXPECT_EQ(expected, json);

    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1::json", expected));
    UEXPECT_NO_THROW(res[0][0].To(json));
    EXPECT_EQ(expected, json);

    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1::jsonb", expected));
    UEXPECT_NO_THROW(res[0][0].To(json));
    EXPECT_EQ(expected, json);
}

UTEST_P(PostgreConnection, JsonStored) {
    CheckConnection(GetConn());

    pg::ResultSet res{nullptr};
    formats::json::Value json;
    auto expected = formats::json::FromString(kJsonText);

    UEXPECT_NO_THROW(
        res = GetConn()
                  ->Execute("select $1, $2", pg::ParameterStore{}.PushBack(expected).PushBack(pg::PlainJson{expected}))
    );
    UEXPECT_NO_THROW(res[0][0].To(json));
    EXPECT_EQ(expected, json);
    UEXPECT_NO_THROW(res[0][1].To(json));
    EXPECT_EQ(expected, json);
}

UTEST_P(PostgreConnection, JsonRawStringSelect) {
    CheckConnection(GetConn());

    /// [json_raw_string_as_set_of]
    auto result = GetConn()->Execute(R"(select '["sale"]'::jsonb)");
    formats::json::RawString raw = result[0].As<formats::json::RawString>();
    EXPECT_EQ(raw.GetView(), R"(["sale"])");
    /// [json_raw_string_as_set_of]

    UEXPECT_NO_THROW(result = GetConn()->Execute(R"(select '["sale"]'::json)"));
    UEXPECT_NO_THROW(raw = result[0].As<formats::json::RawString>());
    EXPECT_EQ(raw.GetView(), R"(["sale"])");

    UEXPECT_NO_THROW(result = GetConn()->Execute(R"(select '{"zulu":3.14}'::jsonb)"));
    UEXPECT_NO_THROW(raw = result[0].As<formats::json::RawString>());
    EXPECT_EQ(raw.GetView(), R"({"zulu": 3.14})");
}

UTEST_P(PostgreConnection, JsonRawStringRoundtrip) {
    CheckConnection(GetConn());

    const formats::json::RawString expected{R"(["sale"])"};
    pg::ResultSet res{nullptr};

    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1", expected));
    formats::json::RawString raw;
    UEXPECT_NO_THROW(raw = res[0].As<formats::json::RawString>());
    EXPECT_EQ(raw, expected);
}

UTEST_P(PostgreConnection, JsonRawStringAsSetOf) {
    CheckConnection(GetConn());

    struct Row {
        formats::json::RawString tags;
    };

    pg::ResultSet res{nullptr};
    UEXPECT_NO_THROW(res = GetConn()->Execute(R"(select '["fast","new"]'::jsonb)"));
    const auto rows = res.AsSetOf<Row>(pg::kRowTag);
    ASSERT_EQ(rows.Size(), 1);
    EXPECT_EQ(rows[0].tags.GetView(), R"(["fast", "new"])");
}

UTEST_P(PostgreConnection, JsonRawStringSurvivesResultDestruction) {
    CheckConnection(GetConn());

    constexpr std::string_view kExpected = R"(["fast", "new"])";
    formats::json::RawString raw;

    {
        const pg::ResultSet res = GetConn()->Execute(R"(select '["fast","new"]'::jsonb)");
        UEXPECT_NO_THROW(res[0][0].To(raw));
    }

    EXPECT_EQ(raw.GetView(), kExpected);

    formats::json::StringBuilder sb;
    formats::json::WriteToStream(raw, sb);
    EXPECT_EQ(sb.GetString(), kExpected);

    struct Row {
        formats::json::RawString tags;
    };

    formats::json::RawString from_row;
    {
        const pg::ResultSet res = GetConn()->Execute(R"(select '["fast","new"]'::jsonb)");
        const auto rows = res.AsSetOf<Row>(pg::kRowTag);
        from_row = rows[0].tags;
    }

    EXPECT_EQ(from_row.GetView(), kExpected);
}

TEST(PostgreIOJson, JsonRawStringWriteToJsonFieldError) {
    const formats::json::RawString raw{R"(["sale"])"};
    pg::test::Buffer buffer;

    UEXPECT_THROW_MSG(
        io::WriteRawBinary(kTypes, buffer, raw, static_cast<pg::Oid>(io::PredefinedOids::kJson)),
        pg::InvalidInputFormat,
        "formats::json::RawString binds as jsonb and cannot be written to a PostgreSQL json "
        "field; explicitly cast to the expected type in SQL query (e.g. `$1::json`)"
    );
}

UTEST_P(PostgreConnection, JsonParse) {
    CheckConnection(GetConn());

    pg::ResultSet res{nullptr};
    formats::json::Value json = formats::json::FromString(kJsonText);

    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1::jsonb", json));
    UEXPECT_NO_THROW(res.AsSingleRow<formats::json::Value>());
    UEXPECT_NO_THROW(res.AsSingleRow<std::string>());

    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1::json", json));
    UEXPECT_NO_THROW(res.AsSingleRow<formats::json::Value>());
    UEXPECT_NO_THROW(res.AsSingleRow<std::string>());
}

}  // namespace

USERVER_NAMESPACE_END
